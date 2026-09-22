#include "nes/cpu.h"

#include <string.h>

void nes_cpu_init(nes_cpu *cpu, nes_cpu_bus bus) {
    memset(cpu, 0, sizeof(*cpu));
    cpu->bus = bus;
    cpu->s = 0xFDu;
    cpu->p = 0x34u; /* I, B and U set */
}

u8 nes_cpu_read8(nes_cpu *cpu, u16 addr) {
    cpu->cycles++;
    return cpu->bus.read(cpu->bus.ctx, addr);
}

void nes_cpu_write8(nes_cpu *cpu, u16 addr, u8 value) {
    cpu->cycles++;
    cpu->bus.write(cpu->bus.ctx, addr, value);
}

static u16 make16(u8 lo, u8 hi) {
    return (u16)(lo | (hi << 8));
}

u16 nes_cpu_read16(nes_cpu *cpu, u16 addr) {
    const u8 lo = nes_cpu_read8(cpu, addr);
    const u8 hi = nes_cpu_read8(cpu, (u16)(addr + 1u));
    return make16(lo, hi);
}

u16 nes_cpu_read16_wrapped(nes_cpu *cpu, u16 addr) {
    const u8 lo = nes_cpu_read8(cpu, addr);
    const u8 hi = nes_cpu_read8(cpu, (u16)((addr & 0xFF00u) | ((addr + 1u) & 0xFFu)));
    return make16(lo, hi);
}

u8 nes_cpu_fetch8(nes_cpu *cpu) {
    return nes_cpu_read8(cpu, cpu->pc++);
}

u16 nes_cpu_fetch16(nes_cpu *cpu) {
    const u8 lo = nes_cpu_fetch8(cpu);
    const u8 hi = nes_cpu_fetch8(cpu);
    return make16(lo, hi);
}

void nes_cpu_push8(nes_cpu *cpu, u8 value) {
    nes_cpu_write8(cpu, (u16)(NES_STACK_BASE | cpu->s), value);
    cpu->s--;
}

void nes_cpu_push16(nes_cpu *cpu, u16 value) {
    nes_cpu_push8(cpu, (u8)(value >> 8));
    nes_cpu_push8(cpu, (u8)value);
}

u8 nes_cpu_pop8(nes_cpu *cpu) {
    cpu->s++;
    return nes_cpu_read8(cpu, (u16)(NES_STACK_BASE | cpu->s));
}

u16 nes_cpu_pop16(nes_cpu *cpu) {
    const u8 lo = nes_cpu_pop8(cpu);
    const u8 hi = nes_cpu_pop8(cpu);
    return make16(lo, hi);
}

static bool pages_differ(u16 a, u16 b) {
    return (a & 0xFF00u) != (b & 0xFF00u);
}

static nes_operand operand(u16 addr, bool page_crossed) {
    return (nes_operand){ .addr = addr, .page_crossed = page_crossed };
}

/* Index is added to the low byte first. The high-byte fix-up cycle reads
   from the unfixed address. */
static nes_operand indexed(nes_cpu *cpu, u16 base, u8 index, nes_access access) {
    const u16 addr = (u16)(base + index);
    const bool crossed = pages_differ(base, addr);

    if (crossed || access == NES_ACCESS_WRITE) {
        (void)nes_cpu_read8(cpu, (u16)((base & 0xFF00u) | (addr & 0xFFu)));
    }
    return operand(addr, crossed);
}

nes_operand nes_cpu_resolve_operand(nes_cpu *cpu, nes_addr_mode mode,
                                    nes_access access) {
    switch (mode) {
        case NES_AM_IMP:
        case NES_AM_ACC:
            (void)nes_cpu_read8(cpu, cpu->pc); /* dummy read, PC doesn't move */
            return operand(0, false);

        case NES_AM_IMM:
            return operand(cpu->pc++, false);

        case NES_AM_ZP0:
            return operand(nes_cpu_fetch8(cpu), false);

        case NES_AM_ZPX:
        case NES_AM_ZPY: {
            const u8 base = nes_cpu_fetch8(cpu);
            const u8 index = (mode == NES_AM_ZPX) ? cpu->x : cpu->y;
            (void)nes_cpu_read8(cpu, base);
            return operand((u8)(base + index), false); /* wraps in page zero */
        }

        case NES_AM_REL: {
            const s8 offset = (s8)nes_cpu_fetch8(cpu);
            const u16 target = (u16)(cpu->pc + offset);
            return operand(target, pages_differ(cpu->pc, target));
        }

        case NES_AM_ABS:
            return operand(nes_cpu_fetch16(cpu), false);

        case NES_AM_ABX:
            return indexed(cpu, nes_cpu_fetch16(cpu), cpu->x, access);

        case NES_AM_ABY:
            return indexed(cpu, nes_cpu_fetch16(cpu), cpu->y, access);

        case NES_AM_IND:
            return operand(nes_cpu_read16_wrapped(cpu, nes_cpu_fetch16(cpu)), false);

        case NES_AM_IZX: {
            const u8 base = nes_cpu_fetch8(cpu);
            (void)nes_cpu_read8(cpu, base);
            return operand(nes_cpu_read16_wrapped(cpu, (u8)(base + cpu->x)), false);
        }

        case NES_AM_IZY: {
            const u16 base = nes_cpu_read16_wrapped(cpu, nes_cpu_fetch8(cpu));
            return indexed(cpu, base, cpu->y, access);
        }

        case NES_AM_COUNT:
            break;
    }
    return operand(0, false);
}

u8 nes_addr_mode_operand_size(nes_addr_mode mode) {
    switch (mode) {
        case NES_AM_ABS:
        case NES_AM_ABX:
        case NES_AM_ABY:
        case NES_AM_IND:
            return 2;
        case NES_AM_IMP:
        case NES_AM_ACC:
        case NES_AM_COUNT:
            return 0;
        default:
            return 1;
    }
}

const char *nes_addr_mode_name(nes_addr_mode mode) {
    static const char *const names[NES_AM_COUNT] = {
        "IMP", "ACC", "IMM", "ZP0", "ZPX", "ZPY", "REL",
        "ABS", "ABX", "ABY", "IND", "IZX", "IZY",
    };
    return (mode < NES_AM_COUNT) ? names[mode] : "???";
}
