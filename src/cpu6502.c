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

/* (zp,X) -- pre-indexed indirect. Zero-page pointer address wraps within
 * the zero page (does not carry into page 1) before the 16-bit pointer is
 * read. */
static uint16_t fetch_indexed_indirect_addr(void) {
    uint8_t zp_ptr = (uint8_t)(read6502(pc++) + x);
    uint16_t lo = read6502(zp_ptr);
    uint16_t hi = read6502((uint8_t)(zp_ptr + 1));
    return (uint16_t)(lo | (hi << 8));
}

/* (zp),Y -- post-indexed indirect. Reads a 16-bit base pointer from zero
 * page, then adds Y (with normal 16-bit carry into the page). Sets
 * *page_crossed for the caller to charge the extra read-side cycle. */
static uint16_t fetch_indirect_indexed_addr(int *page_crossed) {
    uint8_t zp_addr = read6502(pc++);
    uint16_t lo = read6502(zp_addr);
    uint16_t hi = read6502((uint8_t)(zp_addr + 1));
    uint16_t base = (uint16_t)(lo | (hi << 8));
    uint16_t effective = (uint16_t)(base + y);
    *page_crossed = ((base & 0xFF00) != (effective & 0xFF00));
    return effective;
}

/* --- branch helper --- */

static void branch_if(int condition, int8_t offset) {
    clockticks6502 += 2;
    if (condition) {
        pc = (uint16_t)(pc + offset);
        clockticks6502 += 1;
    }
}

/* --- compare helper (shared by CMP/CPX/CPY) --- */

static void compare(uint8_t reg, uint8_t operand) {
    uint16_t diff = (uint16_t)reg - (uint16_t)operand;
    if (reg >= operand) {
        status |= FLAG_CARRY;
    } else {
        status &= (uint8_t)~FLAG_CARRY;
    }
    set_zero_and_sign((uint8_t)diff);
}

/* --- stack helpers --- */

static void push8(uint8_t value) {
    write6502((uint16_t)(0x0100 + sp), value);
    sp--;
}

static uint8_t pull8(void) {
    sp++;
    return read6502((uint16_t)(0x0100 + sp));
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

        case 0x90: { /* BCC */
            int8_t offset = (int8_t)fetch_immediate();
            branch_if(!(status & FLAG_CARRY), offset);
            break;
        }

        case 0xB0: { /* BCS */
            int8_t offset = (int8_t)fetch_immediate();
            branch_if(status & FLAG_CARRY, offset);
            break;
        }

        case 0x10: { /* BPL */
            int8_t offset = (int8_t)fetch_immediate();
            branch_if(!(status & FLAG_SIGN), offset);
            break;
        }

        case 0x30: { /* BMI */
            int8_t offset = (int8_t)fetch_immediate();
            branch_if(status & FLAG_SIGN, offset);
            break;
        }

        case 0x29: /* AND immediate */
            a &= fetch_immediate();
            set_zero_and_sign(a);
            clockticks6502 += 2;
            break;

        case 0x09: /* ORA immediate */
            a |= fetch_immediate();
            set_zero_and_sign(a);
            clockticks6502 += 2;
            break;

        case 0x49: /* EOR immediate */
            a ^= fetch_immediate();
            set_zero_and_sign(a);
            clockticks6502 += 2;
            break;

        case 0xE9: { /* SBC immediate (binary mode only; see ADC note) */
            uint8_t operand = fetch_immediate();
            uint8_t borrow_in = (status & FLAG_CARRY) ? 0 : 1;
            uint16_t diff = (uint16_t)a - operand - borrow_in;

            if ((a ^ operand) & (a ^ (uint8_t)diff) & 0x80) {
                status |= FLAG_OVERFLOW;
            } else {
                status &= (uint8_t)~FLAG_OVERFLOW;
            }

            if (diff <= 0xFF) {
                status |= FLAG_CARRY; /* no borrow occurred */
            } else {
                status &= (uint8_t)~FLAG_CARRY; /* borrow occurred */
            }

            a = (uint8_t)diff;
            set_zero_and_sign(a);
            clockticks6502 += 2;
            break;
        }

        case 0xC9: /* CMP immediate */
            compare(a, fetch_immediate());
            clockticks6502 += 2;
            break;

        case 0xE0: /* CPX immediate */
            compare(x, fetch_immediate());
            clockticks6502 += 2;
            break;

        case 0xC0: /* CPY immediate */
            compare(y, fetch_immediate());
            clockticks6502 += 2;
            break;

        case 0x0A: /* ASL accumulator */
            if (a & 0x80) {
                status |= FLAG_CARRY;
            } else {
                status &= (uint8_t)~FLAG_CARRY;
            }
            a = (uint8_t)(a << 1);
            set_zero_and_sign(a);
            clockticks6502 += 2;
            break;

        case 0x4A: /* LSR accumulator */
            if (a & 0x01) {
                status |= FLAG_CARRY;
            } else {
                status &= (uint8_t)~FLAG_CARRY;
            }
            a = (uint8_t)(a >> 1);
            set_zero_and_sign(a);
            clockticks6502 += 2;
            break;

        case 0x2A: { /* ROL accumulator */
            uint8_t carry_in = (status & FLAG_CARRY) ? 1 : 0;
            if (a & 0x80) {
                status |= FLAG_CARRY;
            } else {
                status &= (uint8_t)~FLAG_CARRY;
            }
            a = (uint8_t)((a << 1) | carry_in);
            set_zero_and_sign(a);
            clockticks6502 += 2;
            break;
        }

        case 0x6A: { /* ROR accumulator */
            uint8_t carry_in = (status & FLAG_CARRY) ? 0x80 : 0;
            if (a & 0x01) {
                status |= FLAG_CARRY;
            } else {
                status &= (uint8_t)~FLAG_CARRY;
            }
            a = (uint8_t)((a >> 1) | carry_in);
            set_zero_and_sign(a);
            clockticks6502 += 2;
            break;
        }

        case 0x48: /* PHA */
            push8(a);
            clockticks6502 += 3;
            break;

        case 0x68: /* PLA */
            a = pull8();
            set_zero_and_sign(a);
            clockticks6502 += 4;
            break;

        case 0x20: { /* JSR absolute */
            uint16_t target = fetch_absolute_addr();
            uint16_t return_addr = (uint16_t)(pc - 1);
            push8((uint8_t)(return_addr >> 8));
            push8((uint8_t)(return_addr & 0xFF));
            pc = target;
            clockticks6502 += 6;
            break;
        }

        case 0x60: { /* RTS */
            uint8_t lo = pull8();
            uint8_t hi = pull8();
            pc = (uint16_t)(((hi << 8) | lo) + 1);
            clockticks6502 += 6;
            break;
        }

        case 0x24: { /* BIT zeropage */
            uint8_t operand = read6502(fetch_zeropage_addr());
            if ((a & operand) == 0) {
                status |= FLAG_ZERO;
            } else {
                status &= (uint8_t)~FLAG_ZERO;
            }
            if (operand & 0x80) {
                status |= FLAG_SIGN;
            } else {
                status &= (uint8_t)~FLAG_SIGN;
            }
            if (operand & 0x40) {
                status |= FLAG_OVERFLOW;
            } else {
                status &= (uint8_t)~FLAG_OVERFLOW;
            }
            clockticks6502 += 3;
            break;
        }

        case 0xE6: { /* INC zeropage */
            uint16_t addr = fetch_zeropage_addr();
            uint8_t value = (uint8_t)(read6502(addr) + 1);
            write6502(addr, value);
            set_zero_and_sign(value);
            clockticks6502 += 5;
            break;
        }

        case 0xC6: { /* DEC zeropage */
            uint16_t addr = fetch_zeropage_addr();
            uint8_t value = (uint8_t)(read6502(addr) - 1);
            write6502(addr, value);
            set_zero_and_sign(value);
            clockticks6502 += 5;
            break;
        }

        case 0x58: /* CLI */
            status &= (uint8_t)~FLAG_INTERRUPT;
            clockticks6502 += 2;
            break;

        case 0x78: /* SEI */
            status |= FLAG_INTERRUPT;
            clockticks6502 += 2;
            break;

        case 0xB8: /* CLV */
            status &= (uint8_t)~FLAG_OVERFLOW;
            clockticks6502 += 2;
            break;

        case 0xA1: /* LDA (zp,X) */
            a = read6502(fetch_indexed_indirect_addr());
            set_zero_and_sign(a);
            clockticks6502 += 6;
            break;

        case 0xB1: { /* LDA (zp),Y */
            int page_crossed;
            a = read6502(fetch_indirect_indexed_addr(&page_crossed));
            set_zero_and_sign(a);
            clockticks6502 += page_crossed ? 6 : 5;
            break;
        }

        case 0x81: /* STA (zp,X) */
            write6502(fetch_indexed_indirect_addr(), a);
            clockticks6502 += 6;
            break;

        case 0x91: { /* STA (zp),Y -- always 6 cycles, no page-cross skip
                        on a write (NMOS 6502 timing). */
            int page_crossed;
            write6502(fetch_indirect_indexed_addr(&page_crossed), a);
            clockticks6502 += 6;
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
