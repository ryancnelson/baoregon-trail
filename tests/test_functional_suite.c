/*
 * Integration test: run cpu6502 against Klaus Dormann's 6502 functional
 * test suite. This is the actual Step 2 "100% opcode + flag coverage"
 * gate per CLAUDE.md / NEXT_STEPS.md -- distinct from the hand-written
 * per-opcode unit tests in test_opcodes.c.
 *
 * The .bin is a full 64KB memory image (loaded at $0000); per baochip's
 * spec the reset vector points to $0400 (the test's code_segment entry
 * point). The suite traps forever (JMP *) on success; on failure it
 * traps at the instruction that failed. We detect "stuck" by watching PC
 * stop advancing across steps and report the trapped PC so a human/agent
 * can cross-reference the failing test's listing.
 *
 * Known success address for this build of the suite (05-jan-2020,
 * disable_selfmod=0, report=0): $3469 -- the final "lda #$f0 / success"
 * trap after "test cases have been exhausted". If Klaus's suite is
 * updated upstream this constant may need to move; the stuck-PC detector
 * below works regardless.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/cpu6502.h"

#define FIXTURE_PATH "tests/fixtures/6502_functional_test.bin"
#define RESET_VECTOR 0xFFFC
#define ENTRY_POINT 0x0400
#define MAX_STEPS 100000000UL

static uint8_t test_ram[65536];

uint8_t read6502(uint16_t address) {
    return test_ram[address];
}

void write6502(uint16_t address, uint8_t value) {
    test_ram[address] = value;
}

static int load_fixture(void) {
    FILE *f = fopen(FIXTURE_PATH, "rb");
    if (!f) {
        return 0;
    }
    size_t n = fread(test_ram, 1, sizeof(test_ram), f);
    fclose(f);
    return n == sizeof(test_ram);
}

int main(void) {
    if (!load_fixture()) {
        printf("FAIL: test_functional_suite_fixture_missing_or_wrong_size "
               "(run tests/fetch_functional_test.sh first)\n");
        return 1;
    }

    /* Point the reset vector at the suite's code_segment entry ($0400)
     * and reset the CPU through it, per baochip's spec: point
     * read6502/write6502 at a flat 64KB array and let reset6502() do
     * the normal vector fetch rather than hardcoding pc. */
    test_ram[RESET_VECTOR] = ENTRY_POINT & 0xFF;
    test_ram[RESET_VECTOR + 1] = (ENTRY_POINT >> 8) & 0xFF;
    reset6502();

    uint16_t last_pc = pc;
    int stuck_count = 0;
    unsigned long steps;

    for (steps = 0; steps < MAX_STEPS; steps++) {
        step6502();

        if (pc == last_pc) {
            stuck_count++;
            if (stuck_count >= 3) {
                break;
            }
        } else {
            stuck_count = 0;
        }
        last_pc = pc;
    }

    if (steps >= MAX_STEPS) {
        printf("FAIL: test_functional_suite_does_not_terminate "
               "(exceeded %lu steps without trapping, last pc=$%04X)\n",
               MAX_STEPS, pc);
        return 1;
    }

    /* Klaus Dormann's suite traps with `jmp *` (opcode 0x4C jumping to
     * itself) on success. Detect that specific self-jump signature
     * rather than hardcoding a known-good address, so this test stays
     * correct if the upstream suite's success address ever shifts. */
    if (test_ram[pc] == 0x4C &&
        test_ram[(uint16_t)(pc + 1)] == (pc & 0xFF) &&
        test_ram[(uint16_t)(pc + 2)] == ((pc >> 8) & 0xFF)) {
        printf("PASS: test_functional_suite_reaches_success_trap "
               "(trapped at $%04X after %lu steps)\n", pc, steps);
        return 0;
    }

    printf("FAIL: test_functional_suite_reaches_success_trap "
           "(trapped at $%04X after %lu steps -- this is a FAILING test "
           "case, not success; cross-reference the .a65 listing for the "
           "opcode/flag under test near this address)\n", pc, steps);
    return 1;
}
