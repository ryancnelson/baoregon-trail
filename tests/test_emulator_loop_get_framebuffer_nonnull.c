/*
 * tests/test_emulator_loop_get_framebuffer_nonnull.c -- unit test verifying
 * baoregon_emulator_get_framebuffer() returns a non-NULL pointer and frame cycle execution.
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "../src/emulator_loop.h"
#include "../src/apple2_mem.h"

int main(void) {
    baoregon_emulator_init();

    const uint16_t *fb = baoregon_emulator_get_framebuffer();
    assert(fb != NULL);

    assert(baoregon_emulator_get_cycles_per_frame() == 17050u);
    assert(baoregon_emulator_get_total_cycles() == 0ULL);

    /* In splash menu, run_frame returns target 17050 cycles but g_total_cycles remains 0 (no CPU exec) */
    uint32_t cycles = baoregon_emulator_run_frame();
    assert(cycles == 17050u);
    assert(baoregon_emulator_get_total_cycles() == 0ULL);

    /* Press SELECT (button 2) to launch game */
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    assert(baoregon_emulator_is_game_running() == 1);

    /* Now run_frame executes 6502 instructions and accumulates total cycles */
    baoregon_emulator_run_frame();
    assert(baoregon_emulator_get_total_cycles() > 0ULL);

    printf("PASS: emulator_loop get_framebuffer non-NULL and frame cycle increment verified\n");
    printf("All tests passed.\n");
    return 0;
}
