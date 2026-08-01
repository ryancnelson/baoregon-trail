/*
 * tests/test_apple2_mem_clear_buttons.c -- Unit test for apple2_mem_clear_button_states.
 */
#include <assert.h>
#include <stdio.h>
#include "apple2_mem.h"

static void test_clear_button_states(void) {
    apple2_mem_reset();
    
    apple2_mem_set_button_state(0, 1);
    apple2_mem_set_button_state(1, 1);
    apple2_mem_set_button_state(2, 1);
    assert(apple2_mem_get_button_state(0) == 1);
    assert(apple2_mem_get_button_state(1) == 1);
    assert(apple2_mem_get_button_state(2) == 1);

    apple2_mem_clear_button_states();
    assert(apple2_mem_get_button_state(0) == 0);
    assert(apple2_mem_get_button_state(1) == 0);
    assert(apple2_mem_get_button_state(2) == 0);

    printf("PASS: test_clear_button_states\n");
}

int main(void) {
    test_clear_button_states();
    return 0;
}
