#include "nes/cart.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* iNES header layout:
     0-3  "NES\x1A"
     4    PRG-ROM size in 16 KiB banks
     5    CHR-ROM size in 8 KiB banks (0 => board uses CHR-RAM)
     6    flags6: mirroring, battery, trainer, four-screen, mapper low nibble
     7    flags7: VS/PlayChoice, NES 2.0 marker, mapper high nibble
     8-15 NES 2.0 fields, or padding on plain iNES */
#define FLAGS6_MIRROR_VERTICAL 0x01u
#define FLAGS6_BATTERY         0x02u
#define FLAGS6_TRAINER         0x04u
#define FLAGS6_FOUR_SCREEN     0x08u

#define NES_PRG_RAM_SIZE KiB(8)

static bool header_has_magic(const u8 *header) {
    return header[0] == 'N' && header[1] == 'E' && header[2] == 'S' &&
           header[3] == 0x1Au;
}

/* A NES 2.0 image sets bits 3:2 of flags7 to 0b10. Plain iNES files from
   older tools sometimes leave garbage (ripper signatures) in bytes 7-15,
   so we only trust flags7's mapper nibble when bytes 12-15 are zero. */
static bool header_is_nes2(const u8 *header) {
    return (header[7] & 0x0Cu) == 0x08u;
}

static bool header_flags7_trustworthy(const u8 *header) {
    return header[12] == 0 && header[13] == 0 && header[14] == 0 &&
           header[15] == 0;
}

static u16 header_mapper(const u8 *header, bool is_nes2) {
    u16 mapper = (u16)(header[6] >> 4);

    if (is_nes2 || header_flags7_trustworthy(header)) {
        mapper |= (u16)(header[7] & 0xF0u);
    }
    if (is_nes2) {
        mapper |= (u16)((u16)(header[8] & 0x0Fu) << 8);
    }
    return mapper;
}

static nes_mirroring header_mirroring(const u8 *header) {
    if (header[6] & FLAGS6_FOUR_SCREEN) {
        return NES_MIRROR_FOUR_SCREEN;
    }
    return (header[6] & FLAGS6_MIRROR_VERTICAL) ? NES_MIRROR_VERTICAL
                                                : NES_MIRROR_HORIZONTAL;
}

nes_cart_result nes_cart_load_memory(nes_cart *cart, const u8 *data, size_t size) {
    memset(cart, 0, sizeof(*cart));

    if (size < NES_INES_HEADER_SIZE) {
        return NES_CART_ERR_TOO_SMALL;
    }
    if (!header_has_magic(data)) {
        return NES_CART_ERR_BAD_MAGIC;
    }

    const bool is_nes2 = header_is_nes2(data);
    const u16 mapper = header_mapper(data, is_nes2);
    if (mapper != 0) {
        return NES_CART_ERR_UNSUPPORTED_MAPPER;
    }

    const size_t prg_size = (size_t)data[4] * NES_PRG_BANK_SIZE;
    const size_t chr_rom_size = (size_t)data[5] * NES_CHR_BANK_SIZE;
    if (prg_size == 0) {
        return NES_CART_ERR_NO_PRG;
    }

    size_t offset = NES_INES_HEADER_SIZE;
    if (data[6] & FLAGS6_TRAINER) {
        /* Trainers are a relic of copier-era dumps; NROM carts have no
           use for one, so skip it rather than map it at $7000. */
        offset += NES_TRAINER_SIZE;
    }

    if (size < offset || size - offset < prg_size + chr_rom_size) {
        return NES_CART_ERR_TRUNCATED;
    }

    const size_t chr_size = (chr_rom_size > 0) ? chr_rom_size : NES_CHR_RAM_SIZE;

    u8 *prg = malloc(prg_size);
    u8 *chr = calloc(1, chr_size);
    u8 *prg_ram = calloc(1, NES_PRG_RAM_SIZE);
    if (prg == NULL || chr == NULL || prg_ram == NULL) {
        free(prg);
        free(chr);
        free(prg_ram);
        return NES_CART_ERR_OUT_OF_MEMORY;
    }

    memcpy(prg, data + offset, prg_size);
    if (chr_rom_size > 0) {
        memcpy(chr, data + offset + prg_size, chr_rom_size);
    }

    cart->prg_rom = prg;
    cart->prg_size = prg_size;
    cart->chr = chr;
    cart->chr_size = chr_size;
    cart->chr_is_ram = (chr_rom_size == 0);
    cart->prg_ram = prg_ram;
    cart->prg_ram_size = NES_PRG_RAM_SIZE;
    cart->mapper = mapper;
    cart->mirroring = header_mirroring(data);
    cart->has_battery = (data[6] & FLAGS6_BATTERY) != 0;
    cart->is_nes2 = is_nes2;

    return NES_CART_OK;
}

nes_cart_result nes_cart_load_file(nes_cart *cart, const char *path) {
    memset(cart, 0, sizeof(*cart));

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return NES_CART_ERR_OPEN;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NES_CART_ERR_READ;
    }
    const long file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        return NES_CART_ERR_READ;
    }
    rewind(file);

    if ((size_t)file_size < NES_INES_HEADER_SIZE) {
        fclose(file);
        return NES_CART_ERR_TOO_SMALL;
    }

    u8 *image = malloc((size_t)file_size);
    if (image == NULL) {
        fclose(file);
        return NES_CART_ERR_OUT_OF_MEMORY;
    }

    const size_t got = fread(image, 1, (size_t)file_size, file);
    fclose(file);

    if (got != (size_t)file_size) {
        free(image);
        return NES_CART_ERR_READ;
    }

    const nes_cart_result result = nes_cart_load_memory(cart, image, got);
    free(image);
    return result;
}

void nes_cart_unload(nes_cart *cart) {
    if (cart == NULL) {
        return;
    }
    free(cart->prg_rom);
    free(cart->chr);
    free(cart->prg_ram);
    memset(cart, 0, sizeof(*cart));
}

const char *nes_cart_result_str(nes_cart_result result) {
    switch (result) {
        case NES_CART_OK:                      return "ok";
        case NES_CART_ERR_OPEN:                return "could not open file";
        case NES_CART_ERR_READ:                return "could not read file";
        case NES_CART_ERR_TOO_SMALL:           return "file is smaller than an iNES header";
        case NES_CART_ERR_BAD_MAGIC:           return "not an iNES image (bad magic)";
        case NES_CART_ERR_TRUNCATED:           return "image is truncated (PRG/CHR data missing)";
        case NES_CART_ERR_NO_PRG:              return "image declares zero PRG-ROM banks";
        case NES_CART_ERR_UNSUPPORTED_MAPPER:  return "unsupported mapper (only NROM/mapper 0 is supported)";
        case NES_CART_ERR_OUT_OF_MEMORY:       return "out of memory";
    }
    return "unknown error";
}

const char *nes_mirroring_str(nes_mirroring mirroring) {
    switch (mirroring) {
        case NES_MIRROR_HORIZONTAL:  return "horizontal";
        case NES_MIRROR_VERTICAL:    return "vertical";
        case NES_MIRROR_FOUR_SCREEN: return "four-screen";
    }
    return "unknown";
}
