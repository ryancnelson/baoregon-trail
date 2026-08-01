/*
 * Follow-up isolation test: the previous test
 * (test_emulator_loop_reset_combo_no_spurious_nav.c) held all 3 buttons
 * simultaneously, where PB0=PREV and PB1=NEXT cancel each other out
 * back to the same slot -- that could have been a false-negative
 * (accidentally landing back at slot 0 by cancellation, not because
 * the edge detector was actually correct). This isolates PB1=NEXT
 * alone, held across a reset, to prove there's no spurious single-
 * direction navigation.
 */
#include <assert.h>
#include <stdio.h>

#include "../src/emulator_loop.h"
#include "../src/apple2_mem.h"
#include "../src/cartridge_layout.h"

int main(void) {
    baoregon_emulator_init();

    /* Enter a game. */
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(2, 0);
    baoregon_emulator_poll_input();
    assert(baoregon_emulator_is_in_splash_menu() == 0);

    /* Reset combo: hold all 3 (this is what actually triggers reset). */
    apple2_mem_set_button_state(0, 1);
    apple2_mem_set_button_state(1, 1);
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    assert(baoregon_emulator_is_in_splash_menu() == 1);

    /* Real driver's next frame: user releases PB0/PB2 first (combo
     * naturally doesn't release all 3 fingers in perfect sync), but
     * PB1 (NEXT) is still held a moment longer. */
    apple2_mem_set_button_state(0, 0);
    apple2_mem_set_button_state(1, 1); /* still held */
    apple2_mem_set_button_state(2, 0);
    baoregon_emulator_poll_input();

    const cartridge_slot_t *slot_after = baoregon_emulator_get_current_slot();
    if (slot_after != &cartridge_slots[0]) {
        fprintf(stderr, "FAIL: splash menu selection spuriously advanced to "
                        "slot index %ld -- PB1 (NEXT) being still-held from "
                        "the reset combo was treated as a fresh press edge\n",
                        (long)(slot_after - cartridge_slots));
        assert(0);
    }
    printf("PASS: test_single_still_held_button_after_combo_reset_does_not_navigate\n");
    printf("All tests passed.\n");
    return 0;
}
