/*
 * tests/test_boot_splash_invalid_index_clamping.c -- unit test verifying
 * boot_splash_current_slot() safely clamps out-of-bounds selected_index
 * (-5, 99) to slot 0, and boot_splash API functions handle NULL inputs safely.
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../src/boot_splash.h"
#include "../src/cartridge_layout.h"

int main(void) {
    boot_splash_state_t state;
    boot_splash_init(&state);

    /* Verify slot 0 default */
    const cartridge_slot_t *slot0 = boot_splash_current_slot(&state);
    assert(slot0 != NULL);
    assert(strcmp(slot0->title, "The Oregon Trail (1985)") == 0);

    /* Mutate selected_index to negative out-of-bounds value */
    state.selected_index = -5;
    const cartridge_slot_t *clamped_neg = boot_splash_current_slot(&state);
    assert(clamped_neg == slot0);

    /* Mutate selected_index to positive out-of-bounds value */
    state.selected_index = 99;
    const cartridge_slot_t *clamped_pos = boot_splash_current_slot(&state);
    assert(clamped_pos == slot0);

    /* NULL state safety checks */
    assert(boot_splash_current_slot(NULL) == NULL);
    assert(boot_splash_handle_button(NULL, BOOT_SPLASH_BUTTON_NEXT, NULL) == 0);

    boot_splash_button_edge_state_t edge;
    boot_splash_button_edge_state_init(&edge);
    assert(boot_splash_poll_apple2_mem_buttons(NULL, &edge, NULL) == 0);
    assert(boot_splash_poll_apple2_mem_buttons(&state, NULL, NULL) == 0);

    printf("PASS: boot_splash invalid index clamping and NULL safety verified\n");
    printf("All tests passed.\n");
    return 0;
}
