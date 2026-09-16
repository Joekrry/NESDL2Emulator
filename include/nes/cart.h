#pragma once

#include "nes/types.h"

/* iNES / NES 2.0 cartridge image loading.

   Only mapper 0 (NROM) is accepted for now; anything else is rejected at
   load time with NES_CART_ERR_UNSUPPORTED_MAPPER so the failure surfaces
   at the ROM path rather than as a mystery read later on. */

#define NES_INES_HEADER_SIZE 16
#define NES_TRAINER_SIZE     512
#define NES_PRG_BANK_SIZE    KiB(16)
#define NES_CHR_BANK_SIZE    KiB(8)

/* Cartridges with CHR-RAM report zero CHR banks; we allocate one 8 KiB
   bank for them, which is all NROM boards ever carry. */
#define NES_CHR_RAM_SIZE     KiB(8)

typedef enum {
    NES_MIRROR_HORIZONTAL = 0,
    NES_MIRROR_VERTICAL,
    NES_MIRROR_FOUR_SCREEN,
} nes_mirroring;

typedef enum {
    NES_CART_OK = 0,
    NES_CART_ERR_OPEN,
    NES_CART_ERR_READ,
    NES_CART_ERR_TOO_SMALL,
    NES_CART_ERR_BAD_MAGIC,
    NES_CART_ERR_TRUNCATED,
    NES_CART_ERR_NO_PRG,
    NES_CART_ERR_UNSUPPORTED_MAPPER,
    NES_CART_ERR_OUT_OF_MEMORY,
} nes_cart_result;

typedef struct {
    u8 *prg_rom;
    size_t prg_size;

    /* Points at either CHR-ROM read from the image or a zeroed CHR-RAM
       bank; chr_is_ram says which, and gates writes from the PPU bus. */
    u8 *chr;
    size_t chr_size;
    bool chr_is_ram;

    u8 *prg_ram;          /* $6000-$7FFF work/save RAM, always present. */
    size_t prg_ram_size;

    u16 mapper;
    nes_mirroring mirroring;
    bool has_battery;
    bool is_nes2;
} nes_cart;

/* Loads an iNES image into `cart`. On success the caller owns the
   allocations and must call nes_cart_unload(). On failure `cart` is left
   zeroed and nothing needs freeing. */
nes_cart_result nes_cart_load_file(nes_cart *cart, const char *path);

/* Same, for an image already in memory (used by tests). */
nes_cart_result nes_cart_load_memory(nes_cart *cart, const u8 *data, size_t size);

void nes_cart_unload(nes_cart *cart);

const char *nes_cart_result_str(nes_cart_result result);
const char *nes_mirroring_str(nes_mirroring mirroring);
