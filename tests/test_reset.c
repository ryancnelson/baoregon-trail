/*
 * RED test #1: reset6502() must load PC from the reset vector at $FFFC/$FFFD.
 *
 * This is the first vertical tracer bullet: before any opcode can execute,
 * the CPU must come out of reset pointed at the right address. Klaus
 * Dormann's test suite relies on this to reach its entry point.
 */
#include <stdio.h>
#include <string.h>
#include "../src/cpu6502.h"

static uint8_t test_ram[65536];

uint8_t read6502(uint16_t address) {
    return test_ram[address];
}

void write6502(uint16_t address, uint8_t value) {
    test_ram[address] = value;
}

int main(void) {
    memset(test_ram, 0, sizeof(test_ram));

    /* Reset vector: low byte at $FFFC, high byte at $FFFD -> PC = $0400 */
    test_ram[0xFFFC] = 0x00;
    test_ram[0xFFFD] = 0x04;

    reset6502();

    if (pc != 0x0400) {
        printf("FAIL: test_reset_vector_sets_pc - expected pc=0x0400, got pc=0x%04X\n", pc);
        return 1;
    }

    printf("PASS: test_reset_vector_sets_pc\n");
    return 0;
}
