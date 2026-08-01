/*
 * RED test: apple2_mem_get_button_state() -- a getter for the pushbutton
 * state apple2_mem_set_button_state() sets, needed so external modules
 * (boot_splash.c's real-hardware button poll adapter) can read current
 * button state without going through the $C061-$C063 soft-switch read
 * path (which is 6502-address-space-shaped, not a clean C API for
 * non-CPU callers like a boot-splash button poller).
 */
#include <stdio.h>
#include "../src/apple2_mem.h"

static int failures = 0;

#define CHECK(cond, label) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", label); \
            failures++; \
        } else { \
            printf("PASS: %s\n", label); \
        } \
    } while (0)

static void test_get_button_state_reflects_set_button_state(void) {
    apple2_mem_reset();

    CHECK(apple2_mem_get_button_state(0) == 0,
          "test_get_button_state_reflects_set_button_state: PB0 starts unpressed");

    apple2_mem_set_button_state(0, 1);
    CHECK(apple2_mem_get_button_state(0) == 1,
          "test_get_button_state_reflects_set_button_state: PB0 pressed reflected");

    apple2_mem_set_button_state(0, 0);
    CHECK(apple2_mem_get_button_state(0) == 0,
          "test_get_button_state_reflects_set_button_state: PB0 released reflected");
}

static void test_get_button_state_covers_all_three_buttons_independently(void) {
    apple2_mem_reset();

    apple2_mem_set_button_state(1, 1);
    CHECK(apple2_mem_get_button_state(0) == 0, "test_get_button_state_covers_all_three_buttons_independently: PB0 untouched");
    CHECK(apple2_mem_get_button_state(1) == 1, "test_get_button_state_covers_all_three_buttons_independently: PB1 pressed");
    CHECK(apple2_mem_get_button_state(2) == 0, "test_get_button_state_covers_all_three_buttons_independently: PB2 untouched");
}

static void test_get_button_state_out_of_range_returns_zero(void) {
    apple2_mem_reset();
    CHECK(apple2_mem_get_button_state(-1) == 0, "test_get_button_state_out_of_range_returns_zero: negative index");
    CHECK(apple2_mem_get_button_state(3) == 0, "test_get_button_state_out_of_range_returns_zero: index 3 (only 0-2 valid)");
}

int main(void) {
    test_get_button_state_reflects_set_button_state();
    test_get_button_state_covers_all_three_buttons_independently();
    test_get_button_state_out_of_range_returns_zero();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
