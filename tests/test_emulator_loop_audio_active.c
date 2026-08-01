/*
 * tests/test_emulator_loop_audio_active.c -- Unit test for baoregon_emulator_is_audio_active.
 */
#include <assert.h>
#include <stdio.h>
#include "emulator_loop.h"
#include "apple2_mem.h"
#include "cpu6502.h"

static void test_audio_active(void) {
    baoregon_emulator_init();
    assert(baoregon_emulator_is_audio_active() == 0);

    /* Accessing $C030 flags a toggle; running a frame applies it to pwm_pin_state */
    read6502(0xC030);
    baoregon_emulator_run_frame();
    assert(baoregon_emulator_is_audio_active() == 1);

    read6502(0xC030);
    baoregon_emulator_run_frame();
    assert(baoregon_emulator_is_audio_active() == 0);

    printf("PASS: test_audio_active\n");
}

int main(void) {
    test_audio_active();
    return 0;
}
