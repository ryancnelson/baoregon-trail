/*
 * tests/test_boot_splash_get_selected_slot_index.c -- unit test for boot_splash_get_selected_slot_index API.
 */
#include <assert.h>
#include <stdio.h>
#include <stddef.h>

#include "../src/boot_splash.h"

int main(void) {
    boot_splash_state_t state;

    /* NULL state safety */
    assert(boot_splash_get_selected_slot_index(NULL) == 0);

    /* Initial state -> slot 0 */
    boot_splash_init(&state);
    assert(boot_splash_get_selected_slot_index(&state) == 0);

    /* NEXT button -> slot 1 */
    boot_splash_handle_button(&state, BOOT_SPLASH_BUTTON_NEXT, NULL);
    assert(boot_splash_get_selected_slot_index(&state) == 1);

    /* Out of bounds negative index safety */
    state.selected_index = -5;
    assert(boot_splash_get_selected_slot_index(&state) == 0);

    /* Out of bounds high index safety */
    state.selected_index = 99;
    assert(boot_splash_get_selected_slot_index(&state) == 0);

    printf("PASS: boot_splash_get_selected_slot_index verified\n");
    printf("All tests passed.\n");
    return 0;
}
