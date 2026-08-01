/*
 * tests/test_emulator_loop_reset_combo.c -- TDD unit tests for 3-button reset combo.
 */
#include <assert.h>
#include <stdio.h>
#include "emulator_loop.h"
#include "apple2_mem.h"

static void test_reset_combo_returns_to_splash_menu(void) {
    baoregon_emulator_init();
    assert(baoregon_emulator_is_in_splash_menu() == 1);

    /* Simulate selecting game 0 by pressing SELECT (button 2) edge */
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(2, 0);
    baoregon_emulator_poll_input();
    assert(baoregon_emulator_is_in_splash_menu() == 0);

    /* Now set all 3 buttons active (PB0, PB1, PB2) */
    apple2_mem_set_button_state(0, 1);
    apple2_mem_set_button_state(1, 1);
    apple2_mem_set_button_state(2, 1);

    /* Poll input during frame */
    baoregon_emulator_poll_input();

    /* Should return to splash menu */
    assert(baoregon_emulator_is_in_splash_menu() == 1);

    printf("PASS: test_reset_combo_returns_to_splash_menu\n");
}

int main(void) {
    test_reset_combo_returns_to_splash_menu();
    return 0;
}
