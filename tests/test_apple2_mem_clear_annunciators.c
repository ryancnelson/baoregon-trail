/*
 * tests/test_apple2_mem_clear_annunciators.c -- Unit test for apple2_mem_clear_annunciator_states.
 */
#include <assert.h>
#include <stdio.h>
#include "apple2_mem.h"
#include "cpu6502.h"

static void test_clear_annunciator_states(void) {
    apple2_mem_reset();
    
    /* Accessing $C059, $C05B, $C05D, $C05F turns ON AN0, AN1, AN2, AN3 */
    read6502(0xC059);
    read6502(0xC05B);
    read6502(0xC05D);
    read6502(0xC05F);

    assert(apple2_mem_get_annunciator_state(0) == 1);
    assert(apple2_mem_get_annunciator_state(1) == 1);
    assert(apple2_mem_get_annunciator_state(2) == 1);
    assert(apple2_mem_get_annunciator_state(3) == 1);

    apple2_mem_clear_annunciator_states();
    assert(apple2_mem_get_annunciator_state(0) == 0);
    assert(apple2_mem_get_annunciator_state(1) == 0);
    assert(apple2_mem_get_annunciator_state(2) == 0);
    assert(apple2_mem_get_annunciator_state(3) == 0);

    printf("PASS: test_clear_annunciator_states\n");
}

int main(void) {
    test_clear_annunciator_states();
    return 0;
}
