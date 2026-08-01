/*
 * test_emulator_loop.c -- Unit tests for the full frame-driven emulator loop.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../src/emulator_loop.h"
#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/boot_splash.h"
#include "../src/video_apple2.h"

static void test_emulator_init_resets_components(void) {
    baoregon_emulator_init();
    assert(apple2_mem_is_text_mode() == 1);
    assert(apple2_mem_is_mixed_mode() == 0);
    assert(apple2_mem_is_page2_selected() == 0);
    assert(apple2_mem_is_hires_mode() == 0);
    printf("PASS: test_emulator_init_resets_components\n");
}

static void test_emulator_run_frame_executes_cycles(void) {
    baoregon_emulator_init();
    uint32_t cycles_executed = baoregon_emulator_run_frame();
    assert(cycles_executed >= BAOREGON_CYCLES_PER_FRAME);
    printf("PASS: test_emulator_run_frame_executes_cycles\n");
}

static void test_emulator_splash_menu_navigation(void) {
    baoregon_emulator_init();
    assert(baoregon_emulator_is_in_splash_menu() == 1);

    /* Inject PB1 (NEXT button press) to select next game */
    apple2_mem_set_button_state(1, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(1, 0);

    /* Inject PB2 (SELECT button press) to launch selected game */
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(2, 0);

    assert(baoregon_emulator_is_in_splash_menu() == 0);
    printf("PASS: test_emulator_splash_menu_navigation\n");
}

int main(void) {
    test_emulator_init_resets_components();
    test_emulator_run_frame_executes_cycles();
    test_emulator_splash_menu_navigation();
    printf("All emulator_loop tests passed cleanly!\n");
    return 0;
}
