/*
 * cpu6502.c -- NMOS 6502 CPU emulation core.
 *
 * TDD skeleton only. Built vertical-tracer-bullet style, one opcode/behavior
 * at a time against Klaus Dormann's 6502_functional_test.bin (see
 * tests/README.md). Do not add behavior here without a failing test first.
 */

#include "cpu6502.h"

uint16_t pc = 0;
uint8_t a = 0, x = 0, y = 0, sp = 0xFD, status = FLAG_CONSTANT;
uint32_t clockticks6502 = 0;

void reset6502(void) {
    pc = read6502(0xFFFC) | (read6502(0xFFFD) << 8);
}

void exec6502(uint32_t tickcount) {
    (void)tickcount;
    /* TODO: not yet implemented. */
}

void step6502(void) {
    /* TODO: not yet implemented. */
}
