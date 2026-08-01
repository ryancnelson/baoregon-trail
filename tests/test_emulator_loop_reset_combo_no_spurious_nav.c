/*
 * RED test: 3-button-combo reset spurious re-trigger bug.
 *
 * Real bug hypothesis: baoregon_emulator_init() (fired by the 3-button
 * combo) resets g_button_pressed[] to 0 via apple2_mem_reset() AND
 * resets boot_splash's edge-detection state (was_pressed[]) to 0 via
 * boot_splash_button_edge_state_init(). But it does NOT clear the
 * caller's notion of "buttons are still physically held" -- if a real
 * driver polls physical GPIO and re-asserts apple2_mem_set_button_state(n, 1)
 * for buttons still held down on the very next frame (which any real
 * driver would, since the buttons genuinely are still down), then
 * boot_splash's edge detector sees now_pressed=1, was_pressed=0 (freshly
 * reset) and treats it as a BRAND NEW press -- immediately firing
 * PREV/NEXT/SELECT on the splash menu the instant it's entered, even
 * though the user never released and re-pressed anything. This is a
 * real UX bug: the reset combo would spuriously navigate/select in the
 * splash menu it just returned to.
 */
#include <assert.h>
#include <stdio.h>

#include "../src/emulator_loop.h"
#include "../src/apple2_mem.h"
#include "../src/cartridge_layout.h"

int main(void) {
    baoregon_emulator_init();
    assert(baoregon_emulator_is_in_splash_menu() == 1);
    assert(baoregon_emulator_get_current_slot() == &cartridge_slots[0]);

    /* Enter a game via SELECT (PB2) edge. */
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(2, 0);
    baoregon_emulator_poll_input();
    assert(baoregon_emulator_is_in_splash_menu() == 0);

    /* User holds all 3 buttons for the reset combo -- this is a real,
     * physically-held press that persists across the reset (a real
     * driver polling GPIO would see the buttons as still down on the
     * very next frame too, since the user hasn't released anything). */
    apple2_mem_set_button_state(0, 1);
    apple2_mem_set_button_state(1, 1);
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    assert(baoregon_emulator_is_in_splash_menu() == 1);

    /* Simulate a real driver's very next frame: the physical buttons
     * are STILL held (user hasn't released the combo yet), so the
     * driver re-asserts the same pressed=1 state it read from GPIO. */
    apple2_mem_set_button_state(0, 1);
    apple2_mem_set_button_state(1, 1);
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();

    /* The splash menu selection must NOT have moved -- still slot 0,
     * PREV/NEXT must not have spuriously fired just because the combo
     * buttons are still physically down from the reset trigger. */
    const cartridge_slot_t *slot_after = baoregon_emulator_get_current_slot();
    if (slot_after != &cartridge_slots[0]) {
        fprintf(stderr, "FAIL: splash menu selection spuriously moved to slot "
                        "index %ld after 3-button-combo reset, even though "
                        "the user never released/re-pressed any button -- "
                        "the held combo buttons were treated as a fresh "
                        "press by boot_splash's edge detector\n",
                        (long)(slot_after - cartridge_slots));
        assert(0);
    }
    printf("PASS: test_no_spurious_navigation_from_held_reset_combo_buttons\n");
    printf("All tests passed.\n");
    return 0;
}
