#include <stdio.h>

#include "../src/bunnie_audio.h"

/*
 * RED test #1 (vertical tracer bullet): prove the basic trigger->poll
 * round-trip works -- one toggle in, one pin flip out, flag cleared after.
 *
 * This is the smallest possible slice of BRAINSTORM.md section 3's contract:
 * main CPU traps $C030 and calls bunnie_audio_trigger_toggle() (fire-and-
 * forget, must not block); BIO Core 1 later calls
 * bunnie_audio_poll_and_apply() to actually flip the PWM pin.
 */

static int test_init_sets_pin_low_and_no_pending_toggle(void) {
    bunnie_audio_state_t state;
    bunnie_audio_init(&state);

    if (state.pwm_pin_state != 0) {
        fprintf(stderr, "FAIL: pwm_pin_state = %u, expected 0 after init\n",
                state.pwm_pin_state);
        return 1;
    }
    if (state.toggle_pending != 0) {
        fprintf(stderr, "FAIL: toggle_pending = %u, expected 0 after init\n",
                state.toggle_pending);
        return 1;
    }
    printf("PASS: test_init_sets_pin_low_and_no_pending_toggle\n");
    return 0;
}

static int test_trigger_toggle_sets_pending_flag_without_flipping_pin(void) {
    bunnie_audio_state_t state;
    bunnie_audio_init(&state);

    bunnie_audio_trigger_toggle(&state);

    /* Trigger is fire-and-forget from the main CPU's perspective: it must
     * raise the pending flag but NOT touch pwm_pin_state directly -- only
     * BIO Core 1's poll does that. This proves the two sides stay decoupled
     * (no blocking work happens on the 6502 loop's side).
     */
    if (state.toggle_pending == 0) {
        fprintf(stderr, "FAIL: toggle_pending = 0, expected nonzero after trigger\n");
        return 1;
    }
    if (state.pwm_pin_state != 0) {
        fprintf(stderr,
                "FAIL: pwm_pin_state = %u, expected 0 (trigger must not flip pin directly)\n",
                state.pwm_pin_state);
        return 1;
    }
    printf("PASS: test_trigger_toggle_sets_pending_flag_without_flipping_pin\n");
    return 0;
}

static int test_poll_flips_pin_and_clears_pending_when_toggle_was_triggered(void) {
    bunnie_audio_state_t state;
    bunnie_audio_init(&state);
    bunnie_audio_trigger_toggle(&state);

    uint8_t pin_after_poll = bunnie_audio_poll_and_apply(&state);

    if (pin_after_poll != 1) {
        fprintf(stderr, "FAIL: poll returned %u, expected 1 (pin flipped from 0)\n",
                pin_after_poll);
        return 1;
    }
    if (state.pwm_pin_state != 1) {
        fprintf(stderr, "FAIL: pwm_pin_state = %u, expected 1 after poll\n",
                state.pwm_pin_state);
        return 1;
    }
    if (state.toggle_pending != 0) {
        fprintf(stderr, "FAIL: toggle_pending = %u, expected 0 after poll clears it\n",
                state.toggle_pending);
        return 1;
    }
    printf("PASS: test_poll_flips_pin_and_clears_pending_when_toggle_was_triggered\n");
    return 0;
}

static int test_poll_is_a_noop_when_no_toggle_pending(void) {
    bunnie_audio_state_t state;
    bunnie_audio_init(&state);
    /* No trigger call -- toggle_pending stays 0. */

    uint8_t pin_after_poll = bunnie_audio_poll_and_apply(&state);

    if (pin_after_poll != 0) {
        fprintf(stderr,
                "FAIL: poll returned %u, expected 0 (no pending toggle -> no-op)\n",
                pin_after_poll);
        return 1;
    }
    printf("PASS: test_poll_is_a_noop_when_no_toggle_pending\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_init_sets_pin_low_and_no_pending_toggle();
    failures += test_trigger_toggle_sets_pending_flag_without_flipping_pin();
    failures += test_poll_flips_pin_and_clears_pending_when_toggle_was_triggered();
    failures += test_poll_is_a_noop_when_no_toggle_pending();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
