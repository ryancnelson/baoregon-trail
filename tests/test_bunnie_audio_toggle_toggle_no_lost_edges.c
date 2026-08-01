/*
 * tests/test_bunnie_audio_toggle_toggle_no_lost_edges.c -- unit test verifying
 * that multiple speaker toggles and polling cycles correctly advance PWM pin state.
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "../src/bunnie_audio.h"

int main(void) {
    bunnie_audio_state_t audio;

    bunnie_audio_init(&audio);
    assert(audio.pwm_pin_state == 0);
    assert(audio.toggle_pending == 0);

    /* 1st toggle: pending=1, pin stays 0 until poll */
    bunnie_audio_trigger_toggle(&audio);
    assert(audio.toggle_pending == 1);
    assert(audio.pwm_pin_state == 0);

    /* 1st poll: pin becomes 1, pending cleared */
    uint8_t pin1 = bunnie_audio_poll_and_apply(&audio);
    assert(pin1 == 1);
    assert(audio.pwm_pin_state == 1);
    assert(audio.toggle_pending == 0);

    /* 2nd toggle: pending=1, pin stays 1 until poll */
    bunnie_audio_trigger_toggle(&audio);
    assert(audio.toggle_pending == 1);
    assert(audio.pwm_pin_state == 1);

    /* 2nd poll: pin becomes 0, pending cleared */
    uint8_t pin2 = bunnie_audio_poll_and_apply(&audio);
    assert(pin2 == 0);
    assert(audio.pwm_pin_state == 0);
    assert(audio.toggle_pending == 0);

    /* Subsequent polls with no pending toggle leave pin at 0 */
    uint8_t pin3 = bunnie_audio_poll_and_apply(&audio);
    assert(pin3 == 0);

    printf("PASS: bunnie_audio toggle-poll-toggle-poll edge transition verified\n");
    printf("All tests passed.\n");
    return 0;
}
