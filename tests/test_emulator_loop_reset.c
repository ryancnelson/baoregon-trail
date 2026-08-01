/*
 * tests/test_emulator_loop_reset.c -- Unit test for baoregon_emulator_reset.
 */
#include <assert.h>
#include <stdio.h>
#include "emulator_loop.h"
#include "apple2_mem.h"

static void test_emulator_loop_reset(void) {
    baoregon_emulator_init();
    
    /* Modify state: press button 0, launch game slot 1 */
    apple2_mem_set_button_state(1, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(1, 0);

    /* Perform canonical reset */
    baoregon_emulator_reset();

    /* Assert state is back to clean initial splash mode */
    assert(baoregon_emulator_is_splash_menu_active() == 1);
    assert(baoregon_emulator_get_total_cycles() == 0ULL);
    assert(baoregon_emulator_is_audio_active() == 0);

    printf("PASS: test_emulator_loop_reset\n");
}

int main(void) {
    test_emulator_loop_reset();
    return 0;
}
