/*
 * RED test: apple2_mem_reset() must clear an ARMED/mid-countdown paddle
 * state, not just the countdown target value. Same drift-risk class the
 * team already fixed once for button/annunciator state (commit
 * 31e30a0, "apple2_mem: reset() reuses clear_button_states/
 * clear_annunciator_states helpers") and once for emulator_loop.c's
 * init()/reset_to_splash() cycle-counter drift -- paddle state
 * (g_paddle_countdown_target/g_paddle_reads_remaining/g_paddle_armed)
 * has no dedicated clear_paddle_states() helper and is only zeroed
 * inline in apple2_mem_reset(), with zero test coverage proving that
 * inline zeroing actually clears a paddle that's mid-countdown (armed)
 * at the moment of reset, as opposed to just the idle default state
 * every other paddle test exercises.
 */
#include <assert.h>
#include <stdio.h>
#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"

static void test_reset_clears_armed_paddle_countdown_mid_flight(void) {
    apple2_mem_reset();

    /* Arm PADDLE0 with a countdown that would still be running (not yet
     * expired) at the moment of reset. */
    apple2_mem_set_paddle_value(0, 10);
    (void)write6502; /* silence unused-include warnings if any */
    /* Trigger $C070 to arm the countdown, then read once (still armed,
     * countdown not yet expired). */
    read6502(0xC070);
    uint8_t mid_flight = read6502(0xC064);
    assert((mid_flight & 0x80) != 0); /* confirm it's actually armed before reset */

    apple2_mem_reset();

    /* After reset, PADDLE0 must report NOT armed (bit 7 clear) even
     * without a fresh $C070 trigger -- the old armed/mid-countdown state
     * must not survive the reset. */
    uint8_t after_reset = read6502(0xC064);
    assert((after_reset & 0x80) == 0);

    printf("PASS: test_reset_clears_armed_paddle_countdown_mid_flight\n");
}

static void test_reset_clears_paddle_countdown_target_value(void) {
    /* Companion check: the countdown TARGET value itself (set via
     * apple2_mem_set_paddle_value) must also reset to 0, not just the
     * armed/reads_remaining live state -- otherwise a post-reset $C070
     * trigger would still use a stale target from before the reset. */
    apple2_mem_reset();
    apple2_mem_set_paddle_value(1, 50);

    apple2_mem_reset();

    /* Re-arm with target 0 implicitly (post-reset target should be 0) --
     * arm via $C070 and confirm it discharges IMMEDIATELY (target 0
     * behavior), not after up to 50 reads (the stale pre-reset target). */
    read6502(0xC070);
    uint8_t got = read6502(0xC065);
    assert((got & 0x80) == 0);

    printf("PASS: test_reset_clears_paddle_countdown_target_value\n");
}

int main(void) {
    test_reset_clears_armed_paddle_countdown_mid_flight();
    test_reset_clears_paddle_countdown_target_value();
    printf("All tests passed.\n");
    return 0;
}
