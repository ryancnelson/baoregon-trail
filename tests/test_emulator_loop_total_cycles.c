/*
 * tests/test_emulator_loop_total_cycles.c -- Unit test for cumulative cycle counter.
 */
#include <assert.h>
#include <stdio.h>
#include "emulator_loop.h"
#include "apple2_mem.h"

static void test_total_cycles(void) {
    baoregon_emulator_init();
    assert(baoregon_emulator_get_total_cycles() == 0ULL);

    /* Frame run in splash menu mode executes 0 CPU cycles */
    baoregon_emulator_run_frame();
    assert(baoregon_emulator_get_total_cycles() == 0ULL);

    /* Press SELECT button (PB2) to select game 0 and start execution */
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(2, 0);

    /* Now run frames in game mode */
    uint64_t before = baoregon_emulator_get_total_cycles();
    baoregon_emulator_run_frame();
    uint64_t after1 = baoregon_emulator_get_total_cycles();
    assert(after1 > before);

    baoregon_emulator_run_frame();
    uint64_t after2 = baoregon_emulator_get_total_cycles();
    assert(after2 > after1);

    printf("PASS: test_total_cycles\n");
}

int main(void) {
    test_total_cycles();
    return 0;
}
