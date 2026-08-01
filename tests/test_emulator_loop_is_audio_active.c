/*
 * tests/test_emulator_loop_is_audio_active.c -- unit test verifying
 * baoregon_emulator_is_audio_active() accurately reflects speaker PWM pin toggling.
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "../src/emulator_loop.h"
#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"

int main(void) {
    baoregon_emulator_init();

    /* Pre-game audio active is 0 */
    assert(baoregon_emulator_is_audio_active() == 0);

    /* Launch game */
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    assert(baoregon_emulator_is_game_running() == 1);

    /* Access $C030 (speaker toggle) */
    read6502(0xC030);

    /* Run frame to let bunnie_audio_poll_and_apply process toggle */
    baoregon_emulator_run_frame();
    assert(baoregon_emulator_is_audio_active() == 1);

    /* Access $C030 again */
    write6502(0xC030, 0x00);

    /* Run frame again */
    baoregon_emulator_run_frame();
    assert(baoregon_emulator_is_audio_active() == 0);

    printf("PASS: emulator_loop is_audio_active speaker toggle state verified\n");
    printf("All tests passed.\n");
    return 0;
}
