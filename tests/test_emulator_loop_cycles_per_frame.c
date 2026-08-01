/*
 * tests/test_emulator_loop_cycles_per_frame.c -- Unit test for baoregon_emulator_get_cycles_per_frame.
 */
#include <assert.h>
#include <stdio.h>
#include "emulator_loop.h"

static void test_cycles_per_frame_getter(void) {
    uint32_t cycles = baoregon_emulator_get_cycles_per_frame();
    assert(cycles == BAOREGON_CYCLES_PER_FRAME);
    assert(cycles == 17050);

    printf("PASS: test_cycles_per_frame_getter\n");
}

int main(void) {
    test_cycles_per_frame_getter();
    return 0;
}
