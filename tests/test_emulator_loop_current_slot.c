/*
 * RED test: baoregon_emulator_get_current_slot() -- exposes which
 * cartridge slot is currently selected, for a "now playing" HUD/debug
 * overlay without reaching into boot_splash.c's internal state.
 */
#include <stdio.h>
#include <string.h>
#include "../src/emulator_loop.h"
#include "../src/cartridge_layout.h"
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

static void press_and_release(int pb_index) {
    apple2_mem_set_button_state(pb_index, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(pb_index, 0);
    baoregon_emulator_poll_input();
}

static void test_current_slot_starts_at_slot_0(void) {
    baoregon_emulator_init();
    const cartridge_slot_t *slot = baoregon_emulator_get_current_slot();
    CHECK(slot != NULL, "test_current_slot_starts_at_slot_0: not NULL");
    CHECK(slot == &cartridge_slots[0],
          "test_current_slot_starts_at_slot_0: points at cartridge_slots[0]");
}

static void test_current_slot_tracks_next_button_navigation(void) {
    baoregon_emulator_init();
    press_and_release(1); /* PB1 = NEXT */

    const cartridge_slot_t *slot = baoregon_emulator_get_current_slot();
    CHECK(slot == &cartridge_slots[1],
          "test_current_slot_tracks_next_button_navigation: advances to slot 1 after NEXT");
}

static void test_current_slot_survives_after_leaving_splash_menu(void) {
    baoregon_emulator_init();
    press_and_release(1); /* NEXT -> slot 1 */
    press_and_release(1); /* NEXT -> slot 2 */
    press_and_release(2); /* SELECT -> leaves splash menu */

    CHECK(!baoregon_emulator_is_in_splash_menu(),
          "test_current_slot_survives_after_leaving_splash_menu: left splash menu");

    const cartridge_slot_t *slot = baoregon_emulator_get_current_slot();
    CHECK(slot == &cartridge_slots[2],
          "test_current_slot_survives_after_leaving_splash_menu: still reports slot 2 after selection");
}

int main(void) {
    test_current_slot_starts_at_slot_0();
    test_current_slot_tracks_next_button_navigation();
    test_current_slot_survives_after_leaving_splash_menu();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
