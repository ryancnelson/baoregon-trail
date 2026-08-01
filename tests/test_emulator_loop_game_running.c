/*
 * tests/test_emulator_loop_game_running.c -- Unit test for baoregon_emulator_is_game_running.
 */
#include <assert.h>
#include <stdio.h>
#include "emulator_loop.h"
#include "apple2_mem.h"

static void test_emulator_loop_game_running(void) {
    baoregon_emulator_init();
    
    /* Post-init, emulator is in splash menu mode, so game is NOT running (returns 0) */
    assert(baoregon_emulator_is_game_running() == 0);

    /* Press Button 2 (SELECT) to launch game slot 0 */
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(2, 0);

    /* Now in game execution mode, so game IS running (returns 1) */
    assert(baoregon_emulator_is_game_running() == 1);

    /* Perform soft-reset back to splash menu */
    baoregon_emulator_reset();
    assert(baoregon_emulator_is_game_running() == 0);

    printf("PASS: test_emulator_loop_game_running\n");
}

int main(void) {
    test_emulator_loop_game_running();
    return 0;
}
