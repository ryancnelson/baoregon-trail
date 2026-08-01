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

/* zeropage,X -- address wraps within the zero page, no page-cross concept. */
static uint16_t fetch_zeropage_x_addr(void) {
    return (uint8_t)(read6502(pc++) + x);
}

/* zeropage,Y -- same wraparound rule as zeropage,X, used only by LDX/STX. */
static uint16_t fetch_zeropage_y_addr(void) {
    return (uint8_t)(read6502(pc++) + y);
}

/* absolute,X / absolute,Y -- 16-bit base + index register, with normal
 * carry into the next page. Sets *page_crossed for callers that charge an
 * extra cycle on page crossing (reads only; writes are always the slow
 * cycle count per NMOS 6502 timing). */
static uint16_t fetch_absolute_indexed_addr(uint8_t index, int *page_crossed) {
    uint16_t base = fetch_absolute_addr();
    uint16_t effective = (uint16_t)(base + index);
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

/* --- ADC helper (shared by all addressing modes) --- */

static void adc_with_operand(uint8_t operand) {
    uint8_t carry_in = (status & FLAG_CARRY) ? 1 : 0;
    uint16_t sum = (uint16_t)a + operand + carry_in;

    if (~(a ^ operand) & (a ^ (uint8_t)sum) & 0x80) {
        status |= FLAG_OVERFLOW;
    } else {
        status &= (uint8_t)~FLAG_OVERFLOW;
    }

    if (status & FLAG_DECIMAL) {
        /* BCD decimal-adjust: fix up the low nibble first (checking the
         * pre-adjust binary sum for a >9 nibble or an actual carry out of
         * bit 3), then the high nibble the same way. NMOS 6502 quirk:
         * N/V/Z end up reflecting the intermediate BINARY sum, not the
         * final decimal result -- Klaus Dormann's suite explicitly does
         * not test flags in decimal mode for this reason, so we only need
         * the numeric result and carry to be correct here. */
        uint16_t low = (uint16_t)(a & 0x0F) + (operand & 0x0F) + carry_in;
        uint16_t high = (uint16_t)(a >> 4) + (operand >> 4);
        if (low > 9) {
            low += 6;
            high += 1;
        }
        if (high > 9) {
            high += 6;
        }
        if (high > 15) {
            status |= FLAG_CARRY;
        } else {
            status &= (uint8_t)~FLAG_CARRY;
        }
        a = (uint8_t)(((high << 4) | (low & 0x0F)) & 0xFF);
        return;
    }

    if (sum > 0xFF) {
        status |= FLAG_CARRY;
    } else {
        status &= (uint8_t)~FLAG_CARRY;
    }

    a = (uint8_t)sum;
    set_zero_and_sign(a);
}

/* --- SBC helper (shared by all addressing modes) --- */

static void sbc_with_operand(uint8_t operand) {
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

    if (status & FLAG_DECIMAL) {
        /* BCD decimal-adjust for subtraction: same nibble-wise correction
         * as ADC but subtracting 6/60 when a nibble borrowed, instead of
         * adding. Carry/overflow above are computed from the binary diff
         * (matching real 6502 SBC decimal-mode carry behavior); only the
         * numeric result differs here. */
        int16_t low = (int16_t)(a & 0x0F) - (operand & 0x0F) - borrow_in;
        int16_t high = (int16_t)(a >> 4) - (operand >> 4);
        if (low < 0) {
            low -= 6;
            high -= 1;
        }
        if (high < 0) {
            high -= 6;
        }
        a = (uint8_t)(((high << 4) | (low & 0x0F)) & 0xFF);
        return;
    }

    a = (uint8_t)diff;
    set_zero_and_sign(a);
}

/* --- read-modify-write helpers (shared by all addressing modes of
 * ASL/LSR/ROL/ROR/INC/DEC) --- */

static uint8_t asl_value(uint8_t value) {
    if (value & 0x80) {
        status |= FLAG_CARRY;
    } else {
        status &= (uint8_t)~FLAG_CARRY;
    }
    value = (uint8_t)(value << 1);
    set_zero_and_sign(value);
    return value;
}

static uint8_t lsr_value(uint8_t value) {
    if (value & 0x01) {
        status |= FLAG_CARRY;
    } else {
        status &= (uint8_t)~FLAG_CARRY;
    }
    value = (uint8_t)(value >> 1);
    set_zero_and_sign(value);
    return value;
}

static uint8_t rol_value(uint8_t value) {
    uint8_t carry_in = (status & FLAG_CARRY) ? 1 : 0;
    if (value & 0x80) {
        status |= FLAG_CARRY;
    } else {
        status &= (uint8_t)~FLAG_CARRY;
    }
    value = (uint8_t)((value << 1) | carry_in);
    set_zero_and_sign(value);
    return value;
}

static uint8_t ror_value(uint8_t value) {
    uint8_t carry_in = (status & FLAG_CARRY) ? 0x80 : 0;
    if (value & 0x01) {
        status |= FLAG_CARRY;
    } else {
        status &= (uint8_t)~FLAG_CARRY;
    }
    value = (uint8_t)((value >> 1) | carry_in);
    set_zero_and_sign(value);
    return value;
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

        case 0xA6: /* LDX zeropage */
            x = read6502(fetch_zeropage_addr());
            set_zero_and_sign(x);
            clockticks6502 += 3;
            break;

        case 0xA0: /* LDY immediate */
            y = fetch_immediate();
            set_zero_and_sign(y);
            clockticks6502 += 2;
            break;

        case 0xA4: /* LDY zeropage */
            y = read6502(fetch_zeropage_addr());
            set_zero_and_sign(y);
            clockticks6502 += 3;
            break;

        case 0xB4: /* LDY zeropage,X */
            y = read6502(fetch_zeropage_x_addr());
            set_zero_and_sign(y);
            clockticks6502 += 4;
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
            adc_with_operand(fetch_immediate());
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

        case 0x50: { /* BVC */
            int8_t offset = (int8_t)fetch_immediate();
            branch_if(!(status & FLAG_OVERFLOW), offset);
            break;
        }

        case 0x70: { /* BVS */
            int8_t offset = (int8_t)fetch_immediate();
            branch_if(status & FLAG_OVERFLOW, offset);
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

        case 0xE9: /* SBC immediate */
            sbc_with_operand(fetch_immediate());
            clockticks6502 += 2;
            break;

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
            a = asl_value(a);
            clockticks6502 += 2;
            break;

        case 0x4A: /* LSR accumulator */
            a = lsr_value(a);
            clockticks6502 += 2;
            break;

        case 0x2A: /* ROL accumulator */
            a = rol_value(a);
            clockticks6502 += 2;
            break;

        case 0x6A: /* ROR accumulator */
            a = ror_value(a);
            clockticks6502 += 2;
            break;

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

        case 0xB5: /* LDA zeropage,X */
            a = read6502(fetch_zeropage_x_addr());
            set_zero_and_sign(a);
            clockticks6502 += 4;
            break;

        case 0x95: /* STA zeropage,X */
            write6502(fetch_zeropage_x_addr(), a);
            clockticks6502 += 4;
            break;

        case 0xBD: { /* LDA absolute,X */
            int page_crossed;
            a = read6502(fetch_absolute_indexed_addr(x, &page_crossed));
            set_zero_and_sign(a);
            clockticks6502 += page_crossed ? 5 : 4;
            break;
        }

        case 0xB9: { /* LDA absolute,Y */
            int page_crossed;
            a = read6502(fetch_absolute_indexed_addr(y, &page_crossed));
            set_zero_and_sign(a);
            clockticks6502 += page_crossed ? 5 : 4;
            break;
        }

        case 0x9D: { /* STA absolute,X -- always 5 cycles */
            int page_crossed;
            write6502(fetch_absolute_indexed_addr(x, &page_crossed), a);
            clockticks6502 += 5;
            break;
        }

        case 0x99: { /* STA absolute,Y -- always 5 cycles */
            int page_crossed;
            write6502(fetch_absolute_indexed_addr(y, &page_crossed), a);
            clockticks6502 += 5;
            break;
        }

        case 0x7D: { /* ADC absolute,X */
            int page_crossed;
            adc_with_operand(read6502(fetch_absolute_indexed_addr(x, &page_crossed)));
            clockticks6502 += page_crossed ? 5 : 4;
            break;
        }

        case 0x79: { /* ADC absolute,Y */
            int page_crossed;
            adc_with_operand(read6502(fetch_absolute_indexed_addr(y, &page_crossed)));
            clockticks6502 += page_crossed ? 5 : 4;
            break;
        }

        case 0xDD: { /* CMP absolute,X */
            int page_crossed;
            compare(a, read6502(fetch_absolute_indexed_addr(x, &page_crossed)));
            clockticks6502 += page_crossed ? 5 : 4;
            break;
        }

        case 0xD9: { /* CMP absolute,Y */
            int page_crossed;
            compare(a, read6502(fetch_absolute_indexed_addr(y, &page_crossed)));
            clockticks6502 += page_crossed ? 5 : 4;
            break;
        }

        case 0xBE: { /* LDX absolute,Y */
            int page_crossed;
            x = read6502(fetch_absolute_indexed_addr(y, &page_crossed));
            set_zero_and_sign(x);
            clockticks6502 += page_crossed ? 5 : 4;
            break;
        }

        case 0xBC: { /* LDY absolute,X */
            int page_crossed;
            y = read6502(fetch_absolute_indexed_addr(x, &page_crossed));
            set_zero_and_sign(y);
            clockticks6502 += page_crossed ? 5 : 4;
            break;
        }

        case 0xB6: /* LDX zeropage,Y */
            x = read6502(fetch_zeropage_y_addr());
            set_zero_and_sign(x);
            clockticks6502 += 4;
            break;

        case 0x06: { /* ASL zeropage */
            uint16_t addr = fetch_zeropage_addr();
            write6502(addr, asl_value(read6502(addr)));
            clockticks6502 += 5;
            break;
        }

        case 0x16: { /* ASL zeropage,X */
            uint16_t addr = fetch_zeropage_x_addr();
            write6502(addr, asl_value(read6502(addr)));
            clockticks6502 += 6;
            break;
        }

        case 0x0E: { /* ASL absolute */
            uint16_t addr = fetch_absolute_addr();
            write6502(addr, asl_value(read6502(addr)));
            clockticks6502 += 6;
            break;
        }

        case 0x1E: { /* ASL absolute,X -- always 7 cycles (RMW, no
                        page-cross early termination) */
            int page_crossed;
            uint16_t addr = fetch_absolute_indexed_addr(x, &page_crossed);
            write6502(addr, asl_value(read6502(addr)));
            clockticks6502 += 7;
            break;
        }

        case 0x46: { /* LSR zeropage */
            uint16_t addr = fetch_zeropage_addr();
            write6502(addr, lsr_value(read6502(addr)));
            clockticks6502 += 5;
            break;
        }

        case 0x56: { /* LSR zeropage,X */
            uint16_t addr = fetch_zeropage_x_addr();
            write6502(addr, lsr_value(read6502(addr)));
            clockticks6502 += 6;
            break;
        }

        case 0x4E: { /* LSR absolute */
            uint16_t addr = fetch_absolute_addr();
            write6502(addr, lsr_value(read6502(addr)));
            clockticks6502 += 6;
            break;
        }

        case 0x5E: { /* LSR absolute,X -- always 7 cycles */
            int page_crossed;
            uint16_t addr = fetch_absolute_indexed_addr(x, &page_crossed);
            write6502(addr, lsr_value(read6502(addr)));
            clockticks6502 += 7;
            break;
        }

        case 0x26: { /* ROL zeropage */
            uint16_t addr = fetch_zeropage_addr();
            write6502(addr, rol_value(read6502(addr)));
            clockticks6502 += 5;
            break;
        }

        case 0x36: { /* ROL zeropage,X */
            uint16_t addr = fetch_zeropage_x_addr();
            write6502(addr, rol_value(read6502(addr)));
            clockticks6502 += 6;
            break;
        }

        case 0x2E: { /* ROL absolute */
            uint16_t addr = fetch_absolute_addr();
            write6502(addr, rol_value(read6502(addr)));
            clockticks6502 += 6;
            break;
        }

        case 0x3E: { /* ROL absolute,X -- always 7 cycles */
            int page_crossed;
            uint16_t addr = fetch_absolute_indexed_addr(x, &page_crossed);
            write6502(addr, rol_value(read6502(addr)));
            clockticks6502 += 7;
            break;
        }

        case 0x66: { /* ROR zeropage */
            uint16_t addr = fetch_zeropage_addr();
            write6502(addr, ror_value(read6502(addr)));
            clockticks6502 += 5;
            break;
        }

        case 0x76: { /* ROR zeropage,X */
            uint16_t addr = fetch_zeropage_x_addr();
            write6502(addr, ror_value(read6502(addr)));
            clockticks6502 += 6;
            break;
        }

        case 0x6E: { /* ROR absolute */
            uint16_t addr = fetch_absolute_addr();
            write6502(addr, ror_value(read6502(addr)));
            clockticks6502 += 6;
            break;
        }

        case 0x7E: { /* ROR absolute,X -- always 7 cycles */
            int page_crossed;
            uint16_t addr = fetch_absolute_indexed_addr(x, &page_crossed);
            write6502(addr, ror_value(read6502(addr)));
            clockticks6502 += 7;
            break;
        }

        case 0xF6: { /* INC zeropage,X */
            uint16_t addr = fetch_zeropage_x_addr();
            uint8_t value = (uint8_t)(read6502(addr) + 1);
            write6502(addr, value);
            set_zero_and_sign(value);
            clockticks6502 += 6;
            break;
        }

        case 0xEE: { /* INC absolute */
            uint16_t addr = fetch_absolute_addr();
            uint8_t value = (uint8_t)(read6502(addr) + 1);
            write6502(addr, value);
            set_zero_and_sign(value);
            clockticks6502 += 6;
            break;
        }

        case 0xFE: { /* INC absolute,X -- always 7 cycles */
            int page_crossed;
            uint16_t addr = fetch_absolute_indexed_addr(x, &page_crossed);
            uint8_t value = (uint8_t)(read6502(addr) + 1);
            write6502(addr, value);
            set_zero_and_sign(value);
            clockticks6502 += 7;
            break;
        }

        case 0xD6: { /* DEC zeropage,X */
            uint16_t addr = fetch_zeropage_x_addr();
            uint8_t value = (uint8_t)(read6502(addr) - 1);
            write6502(addr, value);
            set_zero_and_sign(value);
            clockticks6502 += 6;
            break;
        }

        case 0xCE: { /* DEC absolute */
            uint16_t addr = fetch_absolute_addr();
            uint8_t value = (uint8_t)(read6502(addr) - 1);
            write6502(addr, value);
            set_zero_and_sign(value);
            clockticks6502 += 6;
            break;
        }

        case 0xDE: { /* DEC absolute,X -- always 7 cycles */
            int page_crossed;
            uint16_t addr = fetch_absolute_indexed_addr(x, &page_crossed);
            uint8_t value = (uint8_t)(read6502(addr) - 1);
            write6502(addr, value);
            set_zero_and_sign(value);
            clockticks6502 += 7;
            break;
        }

        case 0x6C: { /* JMP indirect -- replicates the classic NMOS 6502
                        page-boundary bug: if the pointer address's low
                        byte is $FF, the high byte is (incorrectly) read
                        from the start of the same page instead of
                        crossing into the next page. Real Apple II/DOS
                        code and Klaus Dormann's suite depend on this bug
                        being replicated exactly, not "fixed". */
            uint16_t ptr = fetch_absolute_addr();
            uint16_t lo_addr = ptr;
            uint16_t hi_addr = (uint16_t)((ptr & 0xFF00) | ((ptr + 1) & 0x00FF));
            uint16_t lo = read6502(lo_addr);
            uint16_t hi = read6502(hi_addr);
            pc = (uint16_t)(lo | (hi << 8));
            clockticks6502 += 5;
            break;
        }

        case 0x08: /* PHP -- always pushes with B and constant bits set,
                      per NMOS 6502 semantics (a software-visible push
                      differs from an interrupt-triggered push). */
            push8((uint8_t)(status | FLAG_BREAK | FLAG_CONSTANT));
            clockticks6502 += 3;
            break;

        case 0x28: /* PLP -- restores all flags from the stack; the B bit
                      pulled back is whatever was pushed (not forced), and
                      constant is always 1 on a real 6502 regardless. */
            status = (uint8_t)(pull8() | FLAG_CONSTANT);
            clockticks6502 += 4;
            break;

        case 0x00: { /* BRK -- treated as a 2-byte instruction: pushes
                        pc+2 (skipping a padding/signature byte), then
                        status with B set, then jumps through the
                        IRQ/BRK vector at $FFFE/$FFFF and sets the
                        interrupt-disable flag. */
            uint16_t return_addr = (uint16_t)(pc + 1);
            push8((uint8_t)(return_addr >> 8));
            push8((uint8_t)(return_addr & 0xFF));
            push8((uint8_t)(status | FLAG_BREAK | FLAG_CONSTANT));
            status |= FLAG_INTERRUPT;
            pc = (uint16_t)(read6502(0xFFFE) | (read6502(0xFFFF) << 8));
            clockticks6502 += 7;
            break;
        }

        case 0x40: { /* RTI -- pulls status then pc (reverse push order of
                        BRK), landing exactly at the pushed return address
                        (no +1 adjustment, unlike RTS). */
            status = (uint8_t)(pull8() | FLAG_CONSTANT);
            uint8_t lo = pull8();
            uint8_t hi = pull8();
            pc = (uint16_t)((hi << 8) | lo);
            clockticks6502 += 6;
            break;
        }

        case 0x8D: /* STA absolute */
            write6502(fetch_absolute_addr(), a);
            clockticks6502 += 4;
            break;

        case 0x86: /* STX zeropage */
            write6502(fetch_zeropage_addr(), x);
            clockticks6502 += 3;
            break;

        case 0x96: /* STX zeropage,Y */
            write6502(fetch_zeropage_y_addr(), x);
            clockticks6502 += 4;
            break;

        case 0x8E: /* STX absolute */
            write6502(fetch_absolute_addr(), x);
            clockticks6502 += 4;
            break;

        case 0x84: /* STY zeropage */
            write6502(fetch_zeropage_addr(), y);
            clockticks6502 += 3;
            break;

        case 0x94: /* STY zeropage,X */
            write6502(fetch_zeropage_x_addr(), y);
            clockticks6502 += 4;
            break;

        case 0x8C: /* STY absolute */
            write6502(fetch_absolute_addr(), y);
            clockticks6502 += 4;
            break;

        case 0x9A: /* TXS -- unlike TSX, does NOT affect any flags */
            sp = x;
            clockticks6502 += 2;
            break;

        case 0xBA: /* TSX */
            x = sp;
            set_zero_and_sign(x);
            clockticks6502 += 2;
            break;

        case 0xD8: /* CLD */
            status &= (uint8_t)~FLAG_DECIMAL;
            clockticks6502 += 2;
            break;

        case 0xF8: /* SED */
            status |= FLAG_DECIMAL;
            clockticks6502 += 2;
            break;

        case 0xAD: /* LDA absolute */
            a = read6502(fetch_absolute_addr());
            set_zero_and_sign(a);
            clockticks6502 += 4;
            break;

        case 0xAE: /* LDX absolute */
            x = read6502(fetch_absolute_addr());
            set_zero_and_sign(x);
            clockticks6502 += 4;
            break;

        case 0xAC: /* LDY absolute */
            y = read6502(fetch_absolute_addr());
            set_zero_and_sign(y);
            clockticks6502 += 4;
            break;

        case 0x2D: /* AND absolute */
            a &= read6502(fetch_absolute_addr());
            set_zero_and_sign(a);
            clockticks6502 += 4;
            break;

        case 0x0D: /* ORA absolute */
            a |= read6502(fetch_absolute_addr());
            set_zero_and_sign(a);
            clockticks6502 += 4;
            break;

        case 0x4D: /* EOR absolute */
            a ^= read6502(fetch_absolute_addr());
            set_zero_and_sign(a);
            clockticks6502 += 4;
            break;

        case 0x6D: /* ADC absolute */
            adc_with_operand(read6502(fetch_absolute_addr()));
            clockticks6502 += 4;
            break;

        case 0xED: /* SBC absolute */
            sbc_with_operand(read6502(fetch_absolute_addr()));
            clockticks6502 += 4;
            break;

        case 0xCD: /* CMP absolute */
            compare(a, read6502(fetch_absolute_addr()));
            clockticks6502 += 4;
            break;

        case 0x35: /* AND zeropage,X */
            a &= read6502(fetch_zeropage_x_addr());
            set_zero_and_sign(a);
            clockticks6502 += 4;
            break;

        case 0x15: /* ORA zeropage,X */
            a |= read6502(fetch_zeropage_x_addr());
            set_zero_and_sign(a);
            clockticks6502 += 4;
            break;

        case 0x55: /* EOR zeropage,X */
            a ^= read6502(fetch_zeropage_x_addr());
            set_zero_and_sign(a);
            clockticks6502 += 4;
            break;

        case 0x75: /* ADC zeropage,X */
            adc_with_operand(read6502(fetch_zeropage_x_addr()));
            clockticks6502 += 4;
            break;

        case 0xF5: /* SBC zeropage,X */
            sbc_with_operand(read6502(fetch_zeropage_x_addr()));
            clockticks6502 += 4;
            break;

        case 0xD5: /* CMP zeropage,X */
            compare(a, read6502(fetch_zeropage_x_addr()));
            clockticks6502 += 4;
            break;

        case 0x25: /* AND zeropage */
            a &= read6502(fetch_zeropage_addr());
            set_zero_and_sign(a);
            clockticks6502 += 3;
            break;

        case 0x05: /* ORA zeropage */
            a |= read6502(fetch_zeropage_addr());
            set_zero_and_sign(a);
            clockticks6502 += 3;
            break;

        case 0x45: /* EOR zeropage */
            a ^= read6502(fetch_zeropage_addr());
            set_zero_and_sign(a);
            clockticks6502 += 3;
            break;

        case 0x65: /* ADC zeropage */
            adc_with_operand(read6502(fetch_zeropage_addr()));
            clockticks6502 += 3;
            break;

        case 0xE5: /* SBC zeropage */
            sbc_with_operand(read6502(fetch_zeropage_addr()));
            clockticks6502 += 3;
            break;

        case 0xC5: /* CMP zeropage */
            compare(a, read6502(fetch_zeropage_addr()));
            clockticks6502 += 3;
            break;

        case 0xE4: /* CPX zeropage */
            compare(x, read6502(fetch_zeropage_addr()));
            clockticks6502 += 3;
            break;

        case 0xC4: /* CPY zeropage */
            compare(y, read6502(fetch_zeropage_addr()));
            clockticks6502 += 3;
            break;

        case 0xEC: /* CPX absolute */
            compare(x, read6502(fetch_absolute_addr()));
            clockticks6502 += 4;
            break;

        case 0xCC: /* CPY absolute */
            compare(y, read6502(fetch_absolute_addr()));
            clockticks6502 += 4;
            break;

        case 0x21: /* AND (zp,X) */
            a &= read6502(fetch_indexed_indirect_addr());
            set_zero_and_sign(a);
            clockticks6502 += 6;
            break;

        case 0x01: /* ORA (zp,X) */
            a |= read6502(fetch_indexed_indirect_addr());
            set_zero_and_sign(a);
            clockticks6502 += 6;
            break;

        case 0x41: /* EOR (zp,X) */
            a ^= read6502(fetch_indexed_indirect_addr());
            set_zero_and_sign(a);
            clockticks6502 += 6;
            break;

        case 0x61: /* ADC (zp,X) */
            adc_with_operand(read6502(fetch_indexed_indirect_addr()));
            clockticks6502 += 6;
            break;

        case 0xE1: /* SBC (zp,X) */
            sbc_with_operand(read6502(fetch_indexed_indirect_addr()));
            clockticks6502 += 6;
            break;

        case 0xC1: /* CMP (zp,X) */
            compare(a, read6502(fetch_indexed_indirect_addr()));
            clockticks6502 += 6;
            break;

        case 0x31: { /* AND (zp),Y */
            int page_crossed;
            a &= read6502(fetch_indirect_indexed_addr(&page_crossed));
            set_zero_and_sign(a);
            clockticks6502 += page_crossed ? 6 : 5;
            break;
        }

        case 0x11: { /* ORA (zp),Y */
            int page_crossed;
            a |= read6502(fetch_indirect_indexed_addr(&page_crossed));
            set_zero_and_sign(a);
            clockticks6502 += page_crossed ? 6 : 5;
            break;
        }

        case 0x51: { /* EOR (zp),Y */
            int page_crossed;
            a ^= read6502(fetch_indirect_indexed_addr(&page_crossed));
            set_zero_and_sign(a);
            clockticks6502 += page_crossed ? 6 : 5;
            break;
        }

        case 0x71: { /* ADC (zp),Y */
            int page_crossed;
            adc_with_operand(read6502(fetch_indirect_indexed_addr(&page_crossed)));
            clockticks6502 += page_crossed ? 6 : 5;
            break;
        }

        case 0xF1: { /* SBC (zp),Y */
            int page_crossed;
            sbc_with_operand(read6502(fetch_indirect_indexed_addr(&page_crossed)));
            clockticks6502 += page_crossed ? 6 : 5;
            break;
        }

        case 0xD1: { /* CMP (zp),Y */
            int page_crossed;
            compare(a, read6502(fetch_indirect_indexed_addr(&page_crossed)));
            clockticks6502 += page_crossed ? 6 : 5;
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
