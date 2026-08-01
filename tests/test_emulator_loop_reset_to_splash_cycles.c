/*
 * RED test: baoregon_emulator_reset_to_splash() must reset
 * g_total_cycles back to 0, matching baoregon_emulator_init()'s
 * behavior. These two functions are meant to be equivalent "back to
 * splash menu" resets (the 3-button-combo hardware trigger calls
 * init(), while reset_to_splash() is the same operation exposed for
 * other callers/tests) -- but init() zeroes the cumulative cycle
 * counter (baoregon_emulator_get_total_cycles()) while
 * reset_to_splash() currently does not. A caller using
 * reset_to_splash() to return to the menu would see a stale non-zero
 * cycle count carried over from the previous game session, which is
 * inconsistent with what the hardware reset combo produces.
 */
#include <assert.h>
#include <stdio.h>

#include "../src/emulator_loop.h"
#include "../src/apple2_mem.h"

static void run_some_frames_to_accumulate_cycles(void) {
    /* Enter game via SELECT (PB2) so run_frame() actually executes 6502
     * cycles (splash-menu frames don't run exec6502()). */
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(2, 0);
    baoregon_emulator_poll_input();
    assert(baoregon_emulator_is_in_splash_menu() == 0);

    for (int i = 0; i < 3; i++) {
        baoregon_emulator_run_frame();
    }
}

static void test_reset_to_splash_zeroes_total_cycles(void) {
    baoregon_emulator_init();
    run_some_frames_to_accumulate_cycles();

    uint64_t cycles_before_reset = baoregon_emulator_get_total_cycles();
    if (cycles_before_reset == 0) {
        fprintf(stderr, "FAIL: test setup didn't actually accumulate cycles "
                        "(sanity check failed, not testing the real thing)\n");
        assert(0);
    }
    printf("PASS: test_reset_to_splash_zeroes_total_cycles: accumulated %llu cycles before reset\n",
           (unsigned long long)cycles_before_reset);

    baoregon_emulator_reset_to_splash();

    uint64_t cycles_after_reset = baoregon_emulator_get_total_cycles();
    if (cycles_after_reset != 0) {
        fprintf(stderr, "FAIL: baoregon_emulator_reset_to_splash() left total_cycles "
                        "at %llu (stale) instead of resetting to 0, unlike "
                        "baoregon_emulator_init() which does reset it\n",
                        (unsigned long long)cycles_after_reset);
        assert(0);
    }
    printf("PASS: test_reset_to_splash_zeroes_total_cycles: total_cycles is 0 after reset_to_splash()\n");
}

int main(void) {
    test_reset_to_splash_zeroes_total_cycles();
    printf("All tests passed.\n");
    return 0;
}
