#pragma once

#include "nes/types.h"

/* NES CPU: a 6502 without decimal mode. Memory access via bus callbacks only. */

#define NES_VECTOR_NMI   0xFFFAu
#define NES_VECTOR_RESET 0xFFFCu
#define NES_VECTOR_IRQ   0xFFFEu

#define NES_STACK_BASE   0x0100u

enum {
    NES_FLAG_C = 1u << 0,   /* carry */
    NES_FLAG_Z = 1u << 1,   /* zero */
    NES_FLAG_I = 1u << 2,   /* interrupt disable */
    NES_FLAG_D = 1u << 3,   /* decimal, stored but does nothing on the NES */
    NES_FLAG_B = 1u << 4,   /* only exists in the copy pushed to the stack */
    NES_FLAG_U = 1u << 5,   /* always 1 */
    NES_FLAG_V = 1u << 6,   /* overflow */
    NES_FLAG_N = 1u << 7,   /* negative */
};

typedef struct {
    void *ctx;
    u8 (*read)(void *ctx, u16 addr);
    void (*write)(void *ctx, u16 addr, u8 value);
} nes_cpu_bus;

typedef enum {
    NES_AM_IMP = 0, /* CLC         */
    NES_AM_ACC,     /* ASL A       */
    NES_AM_IMM,     /* LDA #$10    */
    NES_AM_ZP0,     /* LDA $10     */
    NES_AM_ZPX,     /* LDA $10,X   */
    NES_AM_ZPY,     /* LDX $10,Y   */
    NES_AM_REL,     /* BNE label   */
    NES_AM_ABS,     /* LDA $1234   */
    NES_AM_ABX,     /* LDA $1234,X */
    NES_AM_ABY,     /* LDA $1234,Y */
    NES_AM_IND,     /* JMP ($1234) */
    NES_AM_IZX,     /* LDA ($10,X) */
    NES_AM_IZY,     /* LDA ($10),Y */
    NES_AM_COUNT,
} nes_addr_mode;

/* Stores and read-modify-writes are writes. Writes always pay the indexing
   cycle; reads pay it only on a page cross. */
typedef enum {
    NES_ACCESS_READ = 0,
    NES_ACCESS_WRITE,
} nes_access;

typedef struct {
    u16 addr;
    bool page_crossed;  /* indexing or the branch target crossed a page */
} nes_operand;

/* Instruction identity. Unofficial opcodes arrive in a later commit. */
typedef enum {
    NES_OP_ADC, NES_OP_AND, NES_OP_ASL, NES_OP_BCC, NES_OP_BCS, NES_OP_BEQ,
    NES_OP_BIT, NES_OP_BMI, NES_OP_BNE, NES_OP_BPL, NES_OP_BRK, NES_OP_BVC,
    NES_OP_BVS, NES_OP_CLC, NES_OP_CLD, NES_OP_CLI, NES_OP_CLV, NES_OP_CMP,
    NES_OP_CPX, NES_OP_CPY, NES_OP_DEC, NES_OP_DEX, NES_OP_DEY, NES_OP_EOR,
    NES_OP_INC, NES_OP_INX, NES_OP_INY, NES_OP_JMP, NES_OP_JSR, NES_OP_LDA,
    NES_OP_LDX, NES_OP_LDY, NES_OP_LSR, NES_OP_NOP, NES_OP_ORA, NES_OP_PHA,
    NES_OP_PHP, NES_OP_PLA, NES_OP_PLP, NES_OP_ROL, NES_OP_ROR, NES_OP_RTI,
    NES_OP_RTS, NES_OP_SBC, NES_OP_SEC, NES_OP_SED, NES_OP_SEI, NES_OP_STA,
    NES_OP_STX, NES_OP_STY, NES_OP_TAX, NES_OP_TAY, NES_OP_TSX, NES_OP_TXA,
    NES_OP_TXS, NES_OP_TYA,
    NES_OP_COUNT,
} nes_op;

typedef struct {
    u8 op;              /* nes_op */
    u8 mode;            /* nes_addr_mode */
    u8 access;          /* nes_access */
    bool official;
} nes_opcode;

typedef struct {
    u8 a;
    u8 x;
    u8 y;
    u8 s;       /* stack pointer into page $01 */
    u8 p;       /* status flags */
    u16 pc;

    u64 cycles;

    bool nmi_pending;   /* edge triggered, latched until serviced */
    bool irq_line;      /* level held by the APU or a mapper */

    nes_cpu_bus bus;
} nes_cpu;

/* Sets the power-on register values. PC stays 0 until reset loads it. */
void nes_cpu_init(nes_cpu *cpu, nes_cpu_bus bus);

/* One bus access per cycle. Each read or write adds one to cycles. */
u8 nes_cpu_read8(nes_cpu *cpu, u16 addr);
void nes_cpu_write8(nes_cpu *cpu, u16 addr, u8 value);
u16 nes_cpu_read16(nes_cpu *cpu, u16 addr);

/* read16 with the high byte kept in the same page. Covers the JMP ($xxFF)
   bug and zero-page pointer wrap. */
u16 nes_cpu_read16_wrapped(nes_cpu *cpu, u16 addr);

u8 nes_cpu_fetch8(nes_cpu *cpu);
u16 nes_cpu_fetch16(nes_cpu *cpu);

void nes_cpu_push8(nes_cpu *cpu, u8 value);
void nes_cpu_push16(nes_cpu *cpu, u16 value);
u8 nes_cpu_pop8(nes_cpu *cpu);
u16 nes_cpu_pop16(nes_cpu *cpu);

/* Consumes the operand bytes and returns the effective address. Includes
   hardware dummy reads to keep cycle counts exact. */
nes_operand nes_cpu_resolve_operand(nes_cpu *cpu, nes_addr_mode mode,
                                    nes_access access);

u8 nes_addr_mode_operand_size(nes_addr_mode mode);
const char *nes_addr_mode_name(nes_addr_mode mode);

/* Loads PC from the reset vector and costs 7 cycles. Call before stepping. */
void nes_cpu_reset(nes_cpu *cpu);

/* Latches an NMI. Holds or releases the IRQ level. */
void nes_cpu_nmi(nes_cpu *cpu);
void nes_cpu_set_irq(nes_cpu *cpu, bool held);

/* Runs one instruction, or services a pending interrupt first. Returns the
   cycles consumed. */
u32 nes_cpu_step(nes_cpu *cpu);

const nes_opcode *nes_cpu_opcode(u8 opcode);
const char *nes_op_name(nes_op op);

static inline bool nes_cpu_flag(const nes_cpu *cpu, u8 flag) {
    return (cpu->p & flag) != 0;
}

static inline void nes_cpu_set_flag(nes_cpu *cpu, u8 flag, bool on) {
    if (on) {
        cpu->p = (u8)(cpu->p | flag);
    } else {
        cpu->p = (u8)(cpu->p & (u8)~flag);
    }
}

static inline void nes_cpu_set_zn(nes_cpu *cpu, u8 value) {
    nes_cpu_set_flag(cpu, NES_FLAG_Z, value == 0);
    nes_cpu_set_flag(cpu, NES_FLAG_N, (value & 0x80u) != 0);
}
