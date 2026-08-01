/*
 * RED test: exec6502(tickcount) -- the coarse-grained "run until at
 * least N cycles have elapsed" entry point emulator_loop.c calls once
 * per frame (BAOREGON_CYCLES_PER_FRAME). Zero test coverage existed for
 * this despite being on the hot path for every single frame the badge
 * renders -- a bug here (infinite loop, off-by-one on the target, or
 * overshoot handling) would hang or desync the whole emulator.
 *
 * Real 6502 semantics: cycles are consumed in whole-instruction chunks
 * (you cannot stop mid-instruction), so exec6502() legitimately runs
 * slightly PAST the requested tickcount when the last instruction that
 * crosses the target takes more than 1 cycle -- this is correct behavior
 * to test for, not a bug.
 */
#include <stdio.h>
#include <string.h>
#include "../src/cpu6502.h"

static uint8_t test_ram[65536];
static int failures = 0;

#define CHECK(cond, label) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", label); \
            failures++; \
        } else { \
            printf("PASS: %s\n", label); \
        } \
    } while (0)

uint8_t read6502(uint16_t address) {
    return test_ram[address];
}

void write6502(uint16_t address, uint8_t value) {
    test_ram[address] = value;
}

static void setup(void) {
    memset(test_ram, 0, sizeof(test_ram));
    test_ram[0xFFFC] = 0x00;
    test_ram[0xFFFD] = 0x04;
    reset6502();
    a = 0;
    x = 0;
    y = 0;
    status = 0;
    clockticks6502 = 0;
}

static void test_exec_runs_at_least_the_requested_ticks(void) {
    /* Fill memory with NOP (2 cycles each) so we can predict exactly how
     * many instructions execute for a given tick budget. */
    setup();
    for (int i = 0; i < 100; i++) {
        test_ram[0x0400 + i] = 0xEA; /* NOP */
    }

    exec6502(10);

    CHECK(clockticks6502 >= 10, "test_exec_runs_at_least_the_requested_ticks");
}

static void test_exec_stops_promptly_when_ticks_divide_evenly(void) {
    /* 10 NOPs at 2 cycles each = exactly 20 ticks -- exec6502(20) must
     * land exactly on the boundary, not overshoot into an 11th NOP. */
    setup();
    for (int i = 0; i < 100; i++) {
        test_ram[0x0400 + i] = 0xEA; /* NOP */
    }

    exec6502(20);

    CHECK(clockticks6502 == 20 && pc == 0x0400 + 10,
          "test_exec_stops_promptly_when_ticks_divide_evenly");
}

static void test_exec_with_zero_tickcount_executes_nothing(void) {
    /* If clockticks6502 already meets/exceeds the target (0), the loop
     * condition should never fire -- exec6502(0) is a pure no-op. */
    setup();
    test_ram[0x0400] = 0xEA; /* NOP -- must NOT execute */

    exec6502(0);

    CHECK(pc == 0x0400 && clockticks6502 == 0,
          "test_exec_with_zero_tickcount_executes_nothing");
}

static void test_exec_advances_pc_by_the_correct_instruction_count(void) {
    /* Cross-check against a mix of instruction widths/cycle-costs (not
     * just NOP) to make sure exec6502() isn't accidentally coupled to a
     * single-opcode assumption. LDA immediate (2 bytes, 2 cycles) x 5. */
    setup();
    for (int i = 0; i < 5; i++) {
        test_ram[0x0400 + i * 2] = 0xA9; /* LDA #imm */
        test_ram[0x0401 + i * 2] = 0x00;
    }

    exec6502(10); /* 5 instructions * 2 cycles each = 10 */

    CHECK(pc == 0x0400 + 10 && clockticks6502 == 10,
          "test_exec_advances_pc_by_the_correct_instruction_count");
}

static void test_exec_accumulates_across_multiple_calls(void) {
    /* emulator_loop.c calls exec6502(BAOREGON_CYCLES_PER_FRAME) once per
     * frame, repeatedly, relying on clockticks6502 accumulating rather
     * than resetting -- verify a second call continues from where the
     * first left off instead of re-running from tick 0. */
    setup();
    for (int i = 0; i < 100; i++) {
        test_ram[0x0400 + i] = 0xEA; /* NOP */
    }

    exec6502(20);
    uint32_t ticks_after_first_call = clockticks6502;
    exec6502(20); /* target is relative to CURRENT clockticks6502 */

    CHECK(clockticks6502 == ticks_after_first_call + 20,
          "test_exec_accumulates_across_multiple_calls");
}

static void test_exec_does_not_hang_when_clockticks6502_is_near_overflow(void) {
    /* clockticks6502 is a uint32_t and NEVER resets during a badge's
     * continuous runtime (only baoregon_emulator_init() resets it to 0,
     * which only happens at cold boot / soft-reset combo) -- it just
     * keeps accumulating every frame forever. After ~4.29 billion cycles
     * (well within reach for a badge meant to run continuously for
     * hours/days at ~1MHz+ emulated clock), clockticks6502 approaches
     * UINT32_MAX. exec6502()'s 'target = clockticks6502 + tickcount'
     * computation can then wrap AROUND past UINT32_MAX to a value
     * SMALLER than clockticks6502 -- if that happens, the loop condition
     * 'while (clockticks6502 < target)' is false on the very first
     * check, and exec6502() silently executes ZERO instructions instead
     * of the requested tick count. For emulator_loop.c that means a
     * single dropped frame becomes a permanently frozen CPU (pc never
     * advances again, every subsequent exec6502() call also computes an
     * overflowed target relative to the same stuck clockticks6502). */
    setup();
    for (int i = 0; i < 100; i++) {
        test_ram[0x0400 + i] = 0xEA; /* NOP */
    }
    clockticks6502 = 0xFFFFFFF0u; /* 16 ticks from wraparound */

    exec6502(20); /* naive clockticks6502+20 wraps to a SMALLER value */

    CHECK(clockticks6502 != 0xFFFFFFF0u && pc != 0x0400,
          "test_exec_does_not_hang_when_clockticks6502_is_near_overflow");
}

int main(void) {
    test_exec_runs_at_least_the_requested_ticks();
    test_exec_stops_promptly_when_ticks_divide_evenly();
    test_exec_with_zero_tickcount_executes_nothing();
    test_exec_advances_pc_by_the_correct_instruction_count();
    test_exec_accumulates_across_multiple_calls();
    test_exec_does_not_hang_when_clockticks6502_is_near_overflow();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
