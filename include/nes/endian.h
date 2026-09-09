#pragma once

#include "nes/types.h"

/* The NES/6502 is natively little-endian, matching essentially every
   host this builds on. These helpers exist to avoid unaligned
   type-punned reads of byte arrays (iNES header fields, interrupt
   vectors) -- not to correct byte order. */

static inline u16 bswap16(u16 v) {
    return (u16)((v << 8) | (v >> 8));
}

static inline u16 read_le16(const u8 *p) {
    return (u16)((u16)p[0] | ((u16)p[1] << 8));
}

static inline void write_le16(u8 *p, u16 v) {
    p[0] = (u8)(v & 0xFFu);
    p[1] = (u8)(v >> 8);
}
