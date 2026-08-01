/*
 * tests/test_boot_splash_null_safety.c -- Unit test for boot_splash NULL pointer & boundary safety.
 */
#include <assert.h>
#include <stdio.h>
#include "boot_splash.h"

static void dummy_setter(const uint8_t *img) {
    (void)img;
}

static void test_boot_splash_handles_null_state_safely(void) {
    /* Passing NULL pointer should not crash */
    boot_splash_init(NULL);

    const cartridge_slot_t *slot = boot_splash_current_slot(NULL);
    assert(slot == NULL);

    int res1 = boot_splash_handle_button(NULL, BOOT_SPLASH_BUTTON_NEXT, dummy_setter);
    assert(res1 == 0);

    boot_splash_button_edge_state_init(NULL);

    boot_splash_state_t state;
    boot_splash_button_edge_state_t edge;
    boot_splash_init(&state);
    boot_splash_button_edge_state_init(&edge);

    int res2 = boot_splash_poll_apple2_mem_buttons(NULL, &edge, dummy_setter);
    assert(res2 == 0);

    int res3 = boot_splash_poll_apple2_mem_buttons(&state, NULL, dummy_setter);
    assert(res3 == 0);

    printf("PASS: test_boot_splash_handles_null_state_safely\n");
}

static void test_boot_splash_current_slot_bounds_protection(void) {
    boot_splash_state_t state;
    state.selected_index = -5;
    const cartridge_slot_t *slot1 = boot_splash_current_slot(&state);
    assert(slot1 != NULL);
    assert(slot1 == &cartridge_slots[0]);

    state.selected_index = 999;
    const cartridge_slot_t *slot2 = boot_splash_current_slot(&state);
    assert(slot2 != NULL);
    assert(slot2 == &cartridge_slots[0]);

    printf("PASS: test_boot_splash_current_slot_bounds_protection\n");
}

int main(void) {
    test_boot_splash_handles_null_state_safely();
    test_boot_splash_current_slot_bounds_protection();
    return 0;
}
