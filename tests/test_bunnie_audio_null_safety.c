/*
 * tests/test_bunnie_audio_null_safety.c -- Unit test for bunnie_audio null pointer safety.
 */
#include <assert.h>
#include <stdio.h>
#include "bunnie_audio.h"

static void test_bunnie_audio_handles_null_state_safely(void) {
    /* Passing NULL pointer should not crash */
    bunnie_audio_init(NULL);
    bunnie_audio_trigger_toggle(NULL);

    uint8_t pin = bunnie_audio_poll_and_apply(NULL);
    assert(pin == 0);

    printf("PASS: test_bunnie_audio_handles_null_state_safely\n");
}

int main(void) {
    test_bunnie_audio_handles_null_state_safely();
    return 0;
}
