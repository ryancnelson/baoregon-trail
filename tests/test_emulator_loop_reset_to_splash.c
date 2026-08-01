/*
 * tests/test_emulator_loop_reset_to_splash.c -- Unit test for baoregon_emulator_reset_to_splash.
 */
#include <assert.h>
#include <stdio.h>
#include "emulator_loop.h"
#include "apple2_mem.h"

static void test_reset_to_splash(void) {
    baoregon_emulator_init();
    assert(baoregon_emulator_is_in_splash_menu() == 1);

    /* Simulate selecting a game (slot 0) by setting button edge state */
    apple2_mem_inject_key(' '); /* clear splash screen by button or key */
    baoregon_emulator_run_frame();

    /* Programmatically invoke reset_to_splash */
    baoregon_emulator_reset_to_splash();

    /* Should be back in splash menu */
    assert(baoregon_emulator_is_in_splash_menu() == 1);

    printf("PASS: test_reset_to_splash\n");
}

int main(void) {
    test_reset_to_splash();
    return 0;
}
