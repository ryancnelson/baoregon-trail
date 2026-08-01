/*
 * cpu6502.c -- NMOS 6502 CPU emulation core.
 *
 * Built vertical-tracer-bullet style: one opcode/behavior at a time, each
 * driven by a failing test in tests/test_opcodes.c before being implemented
 * here (see test-driven-development skill). Do not add opcodes without a
 * failing test first.
 *
 * Bus interface: read6502()/write6502() are provided by the host (Step 2:
 * tests directory flat-array harness; Step 3: apple2_mem.c). This file is
 * bus-topology-agnostic.
 */

#include "cpu6502.h"

uint16_t pc = 0;
uint8_t a = 0, x = 0, y = 0, sp = 0xFD, status = FLAG_CONSTANT;
uint32_t clockticks6502 = 0;

void reset6502(void) {
    pc = read6502(0xFFFC) | (read6502(0xFFFD) << 8);
}

/* --- flag helpers --- */

static void set_zero_and_sign(uint8_t value) {
    if (value == 0) {
        status |= FLAG_ZERO;
    } else {
        status &= (uint8_t)~FLAG_ZERO;
    }
    if (value & 0x80) {
        status |= FLAG_SIGN;
    } else {
        status &= (uint8_t)~FLAG_SIGN;
    }
}

/* --- addressing-mode fetch helpers (each consumes bytes after opcode) --- */

static uint8_t fetch_immediate(void) {
    return read6502(pc++);
}

static uint16_t fetch_zeropage_addr(void) {
    return read6502(pc++);
}

static uint16_t fetch_absolute_addr(void) {
    uint16_t lo = read6502(pc++);
    uint16_t hi = read6502(pc++);
    return (uint16_t)(lo | (hi << 8));
}

/* --- branch helper --- */

static void branch_if(int condition, int8_t offset) {
    clockticks6502 += 2;
    if (condition) {
        pc = (uint16_t)(pc + offset);
        clockticks6502 += 1;
    }
}

void step6502(void) {
    uint8_t opcode = read6502(pc++);

    switch (opcode) {
        case 0xEA: /* NOP */
            clockticks6502 += 2;
            break;

        case 0xA9: /* LDA immediate */
            a = fetch_immediate();
            set_zero_and_sign(a);
            clockticks6502 += 2;
            break;

        case 0xA5: /* LDA zeropage */
            a = read6502(fetch_zeropage_addr());
            set_zero_and_sign(a);
            clockticks6502 += 3;
            break;

        case 0xA2: /* LDX immediate */
            x = fetch_immediate();
            set_zero_and_sign(x);
            clockticks6502 += 2;
            break;

        case 0xA0: /* LDY immediate */
            y = fetch_immediate();
            set_zero_and_sign(y);
            clockticks6502 += 2;
            break;

        case 0x85: /* STA zeropage */
            write6502(fetch_zeropage_addr(), a);
            clockticks6502 += 3;
            break;

        case 0xAA: /* TAX */
            x = a;
            set_zero_and_sign(x);
            clockticks6502 += 2;
            break;

        case 0xA8: /* TAY */
            y = a;
            set_zero_and_sign(y);
            clockticks6502 += 2;
            break;

        case 0x8A: /* TXA */
            a = x;
            set_zero_and_sign(a);
            clockticks6502 += 2;
            break;

        case 0x98: /* TYA */
            a = y;
            set_zero_and_sign(a);
            clockticks6502 += 2;
            break;

        case 0xE8: /* INX */
            x++;
            set_zero_and_sign(x);
            clockticks6502 += 2;
            break;

        case 0xC8: /* INY */
            y++;
            set_zero_and_sign(y);
            clockticks6502 += 2;
            break;

        case 0xCA: /* DEX */
            x--;
            set_zero_and_sign(x);
            clockticks6502 += 2;
            break;

        case 0x88: /* DEY */
            y--;
            set_zero_and_sign(y);
            clockticks6502 += 2;
            break;

        case 0x18: /* CLC */
            status &= (uint8_t)~FLAG_CARRY;
            clockticks6502 += 2;
            break;

        case 0x38: /* SEC */
            status |= FLAG_CARRY;
            clockticks6502 += 2;
            break;

        case 0x69: { /* ADC immediate (binary mode only; decimal mode is a
                        separate future test per NEXT_STEPS.md flag coverage) */
            uint8_t operand = fetch_immediate();
            uint8_t carry_in = (status & FLAG_CARRY) ? 1 : 0;
            uint16_t sum = (uint16_t)a + operand + carry_in;

            if (~(a ^ operand) & (a ^ (uint8_t)sum) & 0x80) {
                status |= FLAG_OVERFLOW;
            } else {
                status &= (uint8_t)~FLAG_OVERFLOW;
            }

            if (sum > 0xFF) {
                status |= FLAG_CARRY;
            } else {
                status &= (uint8_t)~FLAG_CARRY;
            }

            a = (uint8_t)sum;
            set_zero_and_sign(a);
            clockticks6502 += 2;
            break;
        }

        case 0x4C: /* JMP absolute */
            pc = fetch_absolute_addr();
            clockticks6502 += 3;
            break;

        case 0xF0: { /* BEQ */
            int8_t offset = (int8_t)fetch_immediate();
            branch_if(status & FLAG_ZERO, offset);
            break;
        }

        case 0xD0: { /* BNE */
            int8_t offset = (int8_t)fetch_immediate();
            branch_if(!(status & FLAG_ZERO), offset);
            break;
        }

        default:
            /* Unimplemented opcode: not yet driven by a failing test. */
            clockticks6502 += 2;
            break;
    }
}

void exec6502(uint32_t tickcount) {
    uint32_t target = clockticks6502 + tickcount;
    while (clockticks6502 < target) {
        step6502();
    }
}
