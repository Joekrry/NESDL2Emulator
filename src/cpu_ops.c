#include "nes/cpu.h"

/* Official 6502 instruction set. Used AI to note these as this is pure grunt work*/ 

#define OP_R(code, mnemonic, mode) \
    [code] = { NES_OP_##mnemonic, NES_AM_##mode, NES_ACCESS_READ, true }
#define OP_W(code, mnemonic, mode) \
    [code] = { NES_OP_##mnemonic, NES_AM_##mode, NES_ACCESS_WRITE, true }

static const nes_opcode opcode_table[256] = {
    OP_R(0x69, ADC, IMM), OP_R(0x65, ADC, ZP0), OP_R(0x75, ADC, ZPX),
    OP_R(0x6D, ADC, ABS), OP_R(0x7D, ADC, ABX), OP_R(0x79, ADC, ABY),
    OP_R(0x61, ADC, IZX), OP_R(0x71, ADC, IZY),

    OP_R(0x29, AND, IMM), OP_R(0x25, AND, ZP0), OP_R(0x35, AND, ZPX),
    OP_R(0x2D, AND, ABS), OP_R(0x3D, AND, ABX), OP_R(0x39, AND, ABY),
    OP_R(0x21, AND, IZX), OP_R(0x31, AND, IZY),

    OP_R(0x0A, ASL, ACC), OP_W(0x06, ASL, ZP0), OP_W(0x16, ASL, ZPX),
    OP_W(0x0E, ASL, ABS), OP_W(0x1E, ASL, ABX),

    OP_R(0x90, BCC, REL), OP_R(0xB0, BCS, REL), OP_R(0xF0, BEQ, REL),
    OP_R(0x30, BMI, REL), OP_R(0xD0, BNE, REL), OP_R(0x10, BPL, REL),
    OP_R(0x50, BVC, REL), OP_R(0x70, BVS, REL),

    OP_R(0x24, BIT, ZP0), OP_R(0x2C, BIT, ABS),

    OP_R(0x00, BRK, IMP),

    OP_R(0x18, CLC, IMP), OP_R(0xD8, CLD, IMP), OP_R(0x58, CLI, IMP),
    OP_R(0xB8, CLV, IMP),

    OP_R(0xC9, CMP, IMM), OP_R(0xC5, CMP, ZP0), OP_R(0xD5, CMP, ZPX),
    OP_R(0xCD, CMP, ABS), OP_R(0xDD, CMP, ABX), OP_R(0xD9, CMP, ABY),
    OP_R(0xC1, CMP, IZX), OP_R(0xD1, CMP, IZY),

    OP_R(0xE0, CPX, IMM), OP_R(0xE4, CPX, ZP0), OP_R(0xEC, CPX, ABS),
    OP_R(0xC0, CPY, IMM), OP_R(0xC4, CPY, ZP0), OP_R(0xCC, CPY, ABS),

    OP_W(0xC6, DEC, ZP0), OP_W(0xD6, DEC, ZPX), OP_W(0xCE, DEC, ABS),
    OP_W(0xDE, DEC, ABX),
    OP_R(0xCA, DEX, IMP), OP_R(0x88, DEY, IMP),

    OP_R(0x49, EOR, IMM), OP_R(0x45, EOR, ZP0), OP_R(0x55, EOR, ZPX),
    OP_R(0x4D, EOR, ABS), OP_R(0x5D, EOR, ABX), OP_R(0x59, EOR, ABY),
    OP_R(0x41, EOR, IZX), OP_R(0x51, EOR, IZY),

    OP_W(0xE6, INC, ZP0), OP_W(0xF6, INC, ZPX), OP_W(0xEE, INC, ABS),
    OP_W(0xFE, INC, ABX),
    OP_R(0xE8, INX, IMP), OP_R(0xC8, INY, IMP),

    OP_R(0x4C, JMP, ABS), OP_R(0x6C, JMP, IND), OP_R(0x20, JSR, ABS),

    OP_R(0xA9, LDA, IMM), OP_R(0xA5, LDA, ZP0), OP_R(0xB5, LDA, ZPX),
    OP_R(0xAD, LDA, ABS), OP_R(0xBD, LDA, ABX), OP_R(0xB9, LDA, ABY),
    OP_R(0xA1, LDA, IZX), OP_R(0xB1, LDA, IZY),

    OP_R(0xA2, LDX, IMM), OP_R(0xA6, LDX, ZP0), OP_R(0xB6, LDX, ZPY),
    OP_R(0xAE, LDX, ABS), OP_R(0xBE, LDX, ABY),

    OP_R(0xA0, LDY, IMM), OP_R(0xA4, LDY, ZP0), OP_R(0xB4, LDY, ZPX),
    OP_R(0xAC, LDY, ABS), OP_R(0xBC, LDY, ABX),

    OP_R(0x4A, LSR, ACC), OP_W(0x46, LSR, ZP0), OP_W(0x56, LSR, ZPX),
    OP_W(0x4E, LSR, ABS), OP_W(0x5E, LSR, ABX),

    OP_R(0xEA, NOP, IMP),

    OP_R(0x09, ORA, IMM), OP_R(0x05, ORA, ZP0), OP_R(0x15, ORA, ZPX),
    OP_R(0x0D, ORA, ABS), OP_R(0x1D, ORA, ABX), OP_R(0x19, ORA, ABY),
    OP_R(0x01, ORA, IZX), OP_R(0x11, ORA, IZY),

    OP_R(0x48, PHA, IMP), OP_R(0x08, PHP, IMP),
    OP_R(0x68, PLA, IMP), OP_R(0x28, PLP, IMP),

    OP_R(0x2A, ROL, ACC), OP_W(0x26, ROL, ZP0), OP_W(0x36, ROL, ZPX),
    OP_W(0x2E, ROL, ABS), OP_W(0x3E, ROL, ABX),

    OP_R(0x6A, ROR, ACC), OP_W(0x66, ROR, ZP0), OP_W(0x76, ROR, ZPX),
    OP_W(0x6E, ROR, ABS), OP_W(0x7E, ROR, ABX),

    OP_R(0x40, RTI, IMP), OP_R(0x60, RTS, IMP),

    OP_R(0xE9, SBC, IMM), OP_R(0xE5, SBC, ZP0), OP_R(0xF5, SBC, ZPX),
    OP_R(0xED, SBC, ABS), OP_R(0xFD, SBC, ABX), OP_R(0xF9, SBC, ABY),
    OP_R(0xE1, SBC, IZX), OP_R(0xF1, SBC, IZY),

    OP_R(0x38, SEC, IMP), OP_R(0xF8, SED, IMP), OP_R(0x78, SEI, IMP),

    OP_W(0x85, STA, ZP0), OP_W(0x95, STA, ZPX), OP_W(0x8D, STA, ABS),
    OP_W(0x9D, STA, ABX), OP_W(0x99, STA, ABY), OP_W(0x81, STA, IZX),
    OP_W(0x91, STA, IZY),

    OP_W(0x86, STX, ZP0), OP_W(0x96, STX, ZPY), OP_W(0x8E, STX, ABS),
    OP_W(0x84, STY, ZP0), OP_W(0x94, STY, ZPX), OP_W(0x8C, STY, ABS),

    OP_R(0xAA, TAX, IMP), OP_R(0xA8, TAY, IMP), OP_R(0xBA, TSX, IMP),
    OP_R(0x8A, TXA, IMP), OP_R(0x9A, TXS, IMP), OP_R(0x98, TYA, IMP),
};

const nes_opcode *nes_cpu_opcode(u8 opcode) {
    return &opcode_table[opcode];
}

const char *nes_op_name(nes_op op) {
    static const char *const names[NES_OP_COUNT] = {
        "ADC", "AND", "ASL", "BCC", "BCS", "BEQ", "BIT", "BMI", "BNE", "BPL",
        "BRK", "BVC", "BVS", "CLC", "CLD", "CLI", "CLV", "CMP", "CPX", "CPY",
        "DEC", "DEX", "DEY", "EOR", "INC", "INX", "INY", "JMP", "JSR", "LDA",
        "LDX", "LDY", "LSR", "NOP", "ORA", "PHA", "PHP", "PLA", "PLP", "ROL",
        "ROR", "RTI", "RTS", "SBC", "SEC", "SED", "SEI", "STA", "STX", "STY",
        "TAX", "TAY", "TSX", "TXA", "TXS", "TYA",
    };
    return (op < NES_OP_COUNT) ? names[op] : "???";
}

/* P as pushed by BRK and interrupts: B is set only for BRK. */
static u8 status_for_push(const nes_cpu *cpu, bool from_brk) {
    u8 p = (u8)(cpu->p | NES_FLAG_U);
    if (from_brk) {
        p = (u8)(p | NES_FLAG_B);
    } else {
        p = (u8)(p & (u8)~NES_FLAG_B);
    }
    return p;
}

static void pull_status(nes_cpu *cpu) {
    cpu->p = (u8)((nes_cpu_pop8(cpu) & (u8)~NES_FLAG_B) | NES_FLAG_U);
}

/* Shared body of NMI, IRQ and BRK. */
static void enter_interrupt(nes_cpu *cpu, u16 vector, bool from_brk) {
    nes_cpu_push16(cpu, cpu->pc);
    nes_cpu_push8(cpu, status_for_push(cpu, from_brk));
    nes_cpu_set_flag(cpu, NES_FLAG_I, true);
    cpu->pc = nes_cpu_read16(cpu, vector);
}

void nes_cpu_reset(nes_cpu *cpu) {
    /* Five cycles pass before the vector fetch. The three "pushes" only
       decrement S. */
    for (int i = 0; i < 2; i++) {
        (void)nes_cpu_read8(cpu, cpu->pc);
    }
    for (int i = 0; i < 3; i++) {
        (void)nes_cpu_read8(cpu, (u16)(NES_STACK_BASE | cpu->s));
        cpu->s--;
    }

    nes_cpu_set_flag(cpu, NES_FLAG_I, true);
    cpu->nmi_pending = false;
    cpu->pc = nes_cpu_read16(cpu, NES_VECTOR_RESET);
}

void nes_cpu_nmi(nes_cpu *cpu) {
    cpu->nmi_pending = true;
}

void nes_cpu_set_irq(nes_cpu *cpu, bool held) {
    cpu->irq_line = held;
}

static void branch(nes_cpu *cpu, nes_operand operand, bool take) {
    if (!take) {
        return;
    }
    (void)nes_cpu_read8(cpu, cpu->pc);
    if (operand.page_crossed) {
        (void)nes_cpu_read8(cpu, (u16)((cpu->pc & 0xFF00u) | (operand.addr & 0xFFu)));
    }
    cpu->pc = operand.addr;
}

static void compare(nes_cpu *cpu, u8 reg, u8 value) {
    const u16 diff = (u16)(reg - value);
    nes_cpu_set_flag(cpu, NES_FLAG_C, reg >= value);
    nes_cpu_set_zn(cpu, (u8)diff);
}

static void add_with_carry(nes_cpu *cpu, u8 value) {
    const u16 sum = (u16)(cpu->a + value + (nes_cpu_flag(cpu, NES_FLAG_C) ? 1u : 0u));
    const u8 result = (u8)sum;
    /* Overflow when both inputs share a sign that the result does not. */
    const bool overflow = ((cpu->a ^ result) & (value ^ result) & 0x80u) != 0;

    nes_cpu_set_flag(cpu, NES_FLAG_C, sum > 0xFFu);
    nes_cpu_set_flag(cpu, NES_FLAG_V, overflow);
    cpu->a = result;
    nes_cpu_set_zn(cpu, result);
}

/* Reads the operand, or returns A for accumulator mode. */
static u8 load_operand(nes_cpu *cpu, const nes_opcode *entry, nes_operand operand) {
    return (entry->mode == NES_AM_ACC) ? cpu->a : nes_cpu_read8(cpu, operand.addr);
}

/* Read-modify-write writes the unchanged value back first, as hardware does. */
static void store_operand(nes_cpu *cpu, const nes_opcode *entry,
                          nes_operand operand, u8 original, u8 value) {
    if (entry->mode == NES_AM_ACC) {
        cpu->a = value;
        return;
    }
    nes_cpu_write8(cpu, operand.addr, original);
    nes_cpu_write8(cpu, operand.addr, value);
}

static void execute(nes_cpu *cpu, const nes_opcode *entry, nes_operand operand) {
    switch ((nes_op)entry->op) {
        case NES_OP_ADC: add_with_carry(cpu, load_operand(cpu, entry, operand)); break;
        case NES_OP_SBC: add_with_carry(cpu, (u8)~load_operand(cpu, entry, operand)); break;

        case NES_OP_AND:
            cpu->a = (u8)(cpu->a & load_operand(cpu, entry, operand));
            nes_cpu_set_zn(cpu, cpu->a);
            break;
        case NES_OP_EOR:
            cpu->a = (u8)(cpu->a ^ load_operand(cpu, entry, operand));
            nes_cpu_set_zn(cpu, cpu->a);
            break;
        case NES_OP_ORA:
            cpu->a = (u8)(cpu->a | load_operand(cpu, entry, operand));
            nes_cpu_set_zn(cpu, cpu->a);
            break;

        case NES_OP_ASL: {
            const u8 value = load_operand(cpu, entry, operand);
            const u8 result = (u8)(value << 1);
            nes_cpu_set_flag(cpu, NES_FLAG_C, (value & 0x80u) != 0);
            nes_cpu_set_zn(cpu, result);
            store_operand(cpu, entry, operand, value, result);
            break;
        }
        case NES_OP_LSR: {
            const u8 value = load_operand(cpu, entry, operand);
            const u8 result = (u8)(value >> 1);
            nes_cpu_set_flag(cpu, NES_FLAG_C, (value & 0x01u) != 0);
            nes_cpu_set_zn(cpu, result);
            store_operand(cpu, entry, operand, value, result);
            break;
        }
        case NES_OP_ROL: {
            const u8 value = load_operand(cpu, entry, operand);
            const u8 result = (u8)((value << 1) | (nes_cpu_flag(cpu, NES_FLAG_C) ? 1u : 0u));
            nes_cpu_set_flag(cpu, NES_FLAG_C, (value & 0x80u) != 0);
            nes_cpu_set_zn(cpu, result);
            store_operand(cpu, entry, operand, value, result);
            break;
        }
        case NES_OP_ROR: {
            const u8 value = load_operand(cpu, entry, operand);
            const u8 result = (u8)((value >> 1) | (nes_cpu_flag(cpu, NES_FLAG_C) ? 0x80u : 0u));
            nes_cpu_set_flag(cpu, NES_FLAG_C, (value & 0x01u) != 0);
            nes_cpu_set_zn(cpu, result);
            store_operand(cpu, entry, operand, value, result);
            break;
        }

        case NES_OP_BIT: {
            const u8 value = nes_cpu_read8(cpu, operand.addr);
            /* N and V come straight from the memory byte, not the result. */
            nes_cpu_set_flag(cpu, NES_FLAG_Z, (cpu->a & value) == 0);
            nes_cpu_set_flag(cpu, NES_FLAG_V, (value & 0x40u) != 0);
            nes_cpu_set_flag(cpu, NES_FLAG_N, (value & 0x80u) != 0);
            break;
        }

        case NES_OP_BCC: branch(cpu, operand, !nes_cpu_flag(cpu, NES_FLAG_C)); break;
        case NES_OP_BCS: branch(cpu, operand,  nes_cpu_flag(cpu, NES_FLAG_C)); break;
        case NES_OP_BNE: branch(cpu, operand, !nes_cpu_flag(cpu, NES_FLAG_Z)); break;
        case NES_OP_BEQ: branch(cpu, operand,  nes_cpu_flag(cpu, NES_FLAG_Z)); break;
        case NES_OP_BPL: branch(cpu, operand, !nes_cpu_flag(cpu, NES_FLAG_N)); break;
        case NES_OP_BMI: branch(cpu, operand,  nes_cpu_flag(cpu, NES_FLAG_N)); break;
        case NES_OP_BVC: branch(cpu, operand, !nes_cpu_flag(cpu, NES_FLAG_V)); break;
        case NES_OP_BVS: branch(cpu, operand,  nes_cpu_flag(cpu, NES_FLAG_V)); break;

        case NES_OP_BRK:
            /* The padding byte after the opcode is fetched and discarded. */
            cpu->pc++;
            enter_interrupt(cpu, NES_VECTOR_IRQ, true);
            break;

        case NES_OP_CLC: nes_cpu_set_flag(cpu, NES_FLAG_C, false); break;
        case NES_OP_CLD: nes_cpu_set_flag(cpu, NES_FLAG_D, false); break;
        case NES_OP_CLI: nes_cpu_set_flag(cpu, NES_FLAG_I, false); break;
        case NES_OP_CLV: nes_cpu_set_flag(cpu, NES_FLAG_V, false); break;
        case NES_OP_SEC: nes_cpu_set_flag(cpu, NES_FLAG_C, true); break;
        case NES_OP_SED: nes_cpu_set_flag(cpu, NES_FLAG_D, true); break;
        case NES_OP_SEI: nes_cpu_set_flag(cpu, NES_FLAG_I, true); break;

        case NES_OP_CMP: compare(cpu, cpu->a, load_operand(cpu, entry, operand)); break;
        case NES_OP_CPX: compare(cpu, cpu->x, load_operand(cpu, entry, operand)); break;
        case NES_OP_CPY: compare(cpu, cpu->y, load_operand(cpu, entry, operand)); break;

        case NES_OP_DEC: {
            const u8 value = nes_cpu_read8(cpu, operand.addr);
            const u8 result = (u8)(value - 1u);
            nes_cpu_set_zn(cpu, result);
            store_operand(cpu, entry, operand, value, result);
            break;
        }
        case NES_OP_INC: {
            const u8 value = nes_cpu_read8(cpu, operand.addr);
            const u8 result = (u8)(value + 1u);
            nes_cpu_set_zn(cpu, result);
            store_operand(cpu, entry, operand, value, result);
            break;
        }
        case NES_OP_DEX: cpu->x--; nes_cpu_set_zn(cpu, cpu->x); break;
        case NES_OP_DEY: cpu->y--; nes_cpu_set_zn(cpu, cpu->y); break;
        case NES_OP_INX: cpu->x++; nes_cpu_set_zn(cpu, cpu->x); break;
        case NES_OP_INY: cpu->y++; nes_cpu_set_zn(cpu, cpu->y); break;

        case NES_OP_JMP: cpu->pc = operand.addr; break;

        case NES_OP_JSR:
            (void)nes_cpu_read8(cpu, (u16)(NES_STACK_BASE | cpu->s));
            nes_cpu_push16(cpu, (u16)(cpu->pc - 1u)); /* RTS adds the 1 back */
            cpu->pc = operand.addr;
            break;

        case NES_OP_RTS:
            (void)nes_cpu_read8(cpu, (u16)(NES_STACK_BASE | cpu->s));
            cpu->pc = (u16)(nes_cpu_pop16(cpu) + 1u);
            (void)nes_cpu_read8(cpu, cpu->pc);
            break;

        case NES_OP_RTI:
            (void)nes_cpu_read8(cpu, (u16)(NES_STACK_BASE | cpu->s));
            pull_status(cpu);
            cpu->pc = nes_cpu_pop16(cpu);
            break;

        case NES_OP_LDA:
            cpu->a = load_operand(cpu, entry, operand);
            nes_cpu_set_zn(cpu, cpu->a);
            break;
        case NES_OP_LDX:
            cpu->x = load_operand(cpu, entry, operand);
            nes_cpu_set_zn(cpu, cpu->x);
            break;
        case NES_OP_LDY:
            cpu->y = load_operand(cpu, entry, operand);
            nes_cpu_set_zn(cpu, cpu->y);
            break;

        case NES_OP_STA: nes_cpu_write8(cpu, operand.addr, cpu->a); break;
        case NES_OP_STX: nes_cpu_write8(cpu, operand.addr, cpu->x); break;
        case NES_OP_STY: nes_cpu_write8(cpu, operand.addr, cpu->y); break;

        case NES_OP_PHA: nes_cpu_push8(cpu, cpu->a); break;
        case NES_OP_PHP: nes_cpu_push8(cpu, status_for_push(cpu, true)); break;
        case NES_OP_PLA:
            (void)nes_cpu_read8(cpu, (u16)(NES_STACK_BASE | cpu->s));
            cpu->a = nes_cpu_pop8(cpu);
            nes_cpu_set_zn(cpu, cpu->a);
            break;
        case NES_OP_PLP:
            (void)nes_cpu_read8(cpu, (u16)(NES_STACK_BASE | cpu->s));
            pull_status(cpu);
            break;

        case NES_OP_TAX: cpu->x = cpu->a; nes_cpu_set_zn(cpu, cpu->x); break;
        case NES_OP_TAY: cpu->y = cpu->a; nes_cpu_set_zn(cpu, cpu->y); break;
        case NES_OP_TSX: cpu->x = cpu->s; nes_cpu_set_zn(cpu, cpu->x); break;
        case NES_OP_TXA: cpu->a = cpu->x; nes_cpu_set_zn(cpu, cpu->a); break;
        case NES_OP_TYA: cpu->a = cpu->y; nes_cpu_set_zn(cpu, cpu->a); break;
        case NES_OP_TXS: cpu->s = cpu->x; break; /* no flags */

        case NES_OP_NOP:
        case NES_OP_COUNT:
            break;
    }
}

u32 nes_cpu_step(nes_cpu *cpu) {
    const u64 start = cpu->cycles;

    if (cpu->nmi_pending) {
        cpu->nmi_pending = false;
        (void)nes_cpu_read8(cpu, cpu->pc);
        (void)nes_cpu_read8(cpu, cpu->pc);
        enter_interrupt(cpu, NES_VECTOR_NMI, false);
        return (u32)(cpu->cycles - start);
    }

    if (cpu->irq_line && !nes_cpu_flag(cpu, NES_FLAG_I)) {
        (void)nes_cpu_read8(cpu, cpu->pc);
        (void)nes_cpu_read8(cpu, cpu->pc);
        enter_interrupt(cpu, NES_VECTOR_IRQ, false);
        return (u32)(cpu->cycles - start);
    }

    const u8 code = nes_cpu_fetch8(cpu);
    const nes_opcode *entry = nes_cpu_opcode(code);

    /* Unofficial opcodes land in a later commit. Until then they burn the
       two cycles of an implied NOP. */
    if (!entry->official) {
        (void)nes_cpu_read8(cpu, cpu->pc);
        return (u32)(cpu->cycles - start);
    }

    const nes_operand operand =
        nes_cpu_resolve_operand(cpu, (nes_addr_mode)entry->mode, (nes_access)entry->access);
    execute(cpu, entry, operand);

    return (u32)(cpu->cycles - start);
}
