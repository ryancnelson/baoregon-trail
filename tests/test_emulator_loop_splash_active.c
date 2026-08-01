/*
 * tests/test_emulator_loop_splash_active.c -- Unit test for baoregon_emulator_is_splash_menu_active.
 */
#include <assert.h>
#include <stdio.h>
#include "emulator_loop.h"
#include "apple2_mem.h"

static void test_emulator_loop_splash_active(void) {
    baoregon_emulator_init();
    
    /* Post-init, emulator should be in splash menu mode (returns 1) */
    assert(baoregon_emulator_is_splash_menu_active() == 1);

    /* Press Button 2 (SELECT) to launch game slot 0 */
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(2, 0);

    /* Now in game execution mode (returns 0) */
    assert(baoregon_emulator_is_splash_menu_active() == 0);

    /* Perform soft-reset back to splash menu */
    baoregon_emulator_reset_to_splash();
    assert(baoregon_emulator_is_splash_menu_active() == 1);

    printf("PASS: test_emulator_loop_splash_active\n");
}

int main(void) {
    test_emulator_loop_splash_active();
    return 0;
}
