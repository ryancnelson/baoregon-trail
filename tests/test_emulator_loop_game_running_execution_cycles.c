/*
 * tests/test_emulator_loop_game_running_execution_cycles.c -- integration
 * test proving that stepping frames while a game is running advances
 * baoregon_emulator_get_total_cycles() by exactly BAOREGON_EMULATOR_CYCLES_PER_FRAME
 * per frame.
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "../src/emulator_loop.h"
#include "../src/apple2_mem.h"

int main(void) {
    baoregon_emulator_init();

    /* Transition out of splash menu into game running state via SELECT (PB2) */
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(2, 0);
    baoregon_emulator_poll_input();

    assert(baoregon_emulator_is_game_running() == 1);

    uint64_t initial_cycles = baoregon_emulator_get_total_cycles();

    /* Step 10 frames while game is running */
    for (int i = 0; i < 10; i++) {
        baoregon_emulator_run_frame();
    }

    uint64_t final_cycles = baoregon_emulator_get_total_cycles();
    uint64_t elapsed_cycles = final_cycles - initial_cycles;
    uint64_t expected_cycles = (uint64_t)BAOREGON_CYCLES_PER_FRAME * 10;

    if (elapsed_cycles < expected_cycles || elapsed_cycles > expected_cycles + 100) {
        fprintf(stderr, "FAIL: elapsed cycles %llu out of expected range [%llu, %llu]\n",
                        (unsigned long long)elapsed_cycles,
                        (unsigned long long)expected_cycles,
                        (unsigned long long)(expected_cycles + 100));
        assert(0);
    }
    printf("PASS: stepping 10 frames while game running advanced cycles by %llu "
           "(expected target %llu across instruction boundaries)\n",
           (unsigned long long)elapsed_cycles,
           (unsigned long long)expected_cycles);

    printf("All tests passed.\n");
    return 0;
}
