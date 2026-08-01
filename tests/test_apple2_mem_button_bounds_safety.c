/*
 * tests/test_apple2_mem_button_bounds_safety.c -- unit test verifying
 * button getters/setters out-of-bounds index handling (-1, 3, 99).
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "../src/apple2_mem.h"

int main(void) {
    apple2_mem_reset();

    /* Set valid buttons */
    apple2_mem_set_button_state(0, 1);
    apple2_mem_set_button_state(1, 1);
    apple2_mem_set_button_state(2, 1);

    assert(apple2_mem_get_button_state(0) == 1);
    assert(apple2_mem_get_button_state(1) == 1);
    assert(apple2_mem_get_button_state(2) == 1);

    /* Out of bounds set calls must be silently ignored and not corrupt memory/valid buttons */
    apple2_mem_set_button_state(-1, 0);
    apple2_mem_set_button_state(3, 0);
    apple2_mem_set_button_state(99, 0);

    assert(apple2_mem_get_button_state(0) == 1);
    assert(apple2_mem_get_button_state(1) == 1);
    assert(apple2_mem_get_button_state(2) == 1);

    /* Out of bounds get calls must safely return 0 */
    assert(apple2_mem_get_button_state(-1) == 0);
    assert(apple2_mem_get_button_state(3) == 0);
    assert(apple2_mem_get_button_state(99) == 0);

    printf("PASS: apple2_mem button bounds safety verified for -1, 3, 99\n");
    printf("All tests passed.\n");
    return 0;
}
