/*
 * RED test: boot_splash.c's PREV/NEXT/SELECT cartridge cycling state
 * machine, driven against the real cartridge_slots[] table
 * (cartridge_layout.h) and a mock disk-image-setter matching
 * disk_trap_set_image()'s exact signature (disk_trap.h) so the SELECT
 * path is proven callable with the real function pointer type before
 * wiring to real hardware button GPIO polling.
 */
#include <stdio.h>
#include <string.h>
#include "../src/boot_splash.h"
#include "../src/cartridge_layout.h"
#include "../src/disk_trap.h"

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

static const uint8_t *g_last_selected_image = 0;
static int g_setter_call_count = 0;

static void mock_disk_image_setter(const uint8_t *image) {
    g_last_selected_image = image;
    g_setter_call_count++;
}

static void reset_mock_setter(void) {
    g_last_selected_image = 0;
    g_setter_call_count = 0;
}

static void test_init_selects_first_slot(void) {
    boot_splash_state_t state;
    boot_splash_init(&state);

    CHECK(state.selected_index == 0, "test_init_selects_first_slot: index");
    const cartridge_slot_t *slot = boot_splash_current_slot(&state);
    CHECK(slot == &cartridge_slots[0], "test_init_selects_first_slot: current_slot points at slot 0");
    CHECK(strcmp(slot->title, cartridge_slots[0].title) == 0,
          "test_init_selects_first_slot: title matches");
}

static void test_next_advances_one_slot(void) {
    boot_splash_state_t state;
    boot_splash_init(&state);

    int reset_needed = boot_splash_handle_button(&state, BOOT_SPLASH_BUTTON_NEXT, 0);
    CHECK(reset_needed == 0, "test_next_advances_one_slot: NEXT does not trigger reset");
    CHECK(state.selected_index == 1, "test_next_advances_one_slot: index advances to 1");
}

static void test_prev_moves_back_one_slot(void) {
    boot_splash_state_t state;
    boot_splash_init(&state);
    boot_splash_handle_button(&state, BOOT_SPLASH_BUTTON_NEXT, 0); /* index 1 */
    boot_splash_handle_button(&state, BOOT_SPLASH_BUTTON_NEXT, 0); /* index 2 */

    boot_splash_handle_button(&state, BOOT_SPLASH_BUTTON_PREV, 0); /* back to index 1 */
    CHECK(state.selected_index == 1, "test_prev_moves_back_one_slot");
}

static void test_next_wraps_past_last_slot(void) {
    boot_splash_state_t state;
    boot_splash_init(&state);

    /* Advance to the last slot, then one more NEXT must wrap to 0. */
    for (int i = 0; i < CARTRIDGE_SLOT_COUNT - 1; i++) {
        boot_splash_handle_button(&state, BOOT_SPLASH_BUTTON_NEXT, 0);
    }
    CHECK(state.selected_index == CARTRIDGE_SLOT_COUNT - 1,
          "test_next_wraps_past_last_slot: reaches last slot first");

    boot_splash_handle_button(&state, BOOT_SPLASH_BUTTON_NEXT, 0);
    CHECK(state.selected_index == 0,
          "test_next_wraps_past_last_slot: wraps to slot 0");
}

static void test_prev_wraps_before_first_slot(void) {
    boot_splash_state_t state;
    boot_splash_init(&state);

    /* At index 0, PREV must wrap to the last slot. */
    boot_splash_handle_button(&state, BOOT_SPLASH_BUTTON_PREV, 0);
    CHECK(state.selected_index == CARTRIDGE_SLOT_COUNT - 1,
          "test_prev_wraps_before_first_slot");
}

static void test_select_invokes_setter_with_current_slots_reram_address(void) {
    boot_splash_state_t state;
    boot_splash_init(&state);
    reset_mock_setter();

    boot_splash_handle_button(&state, BOOT_SPLASH_BUTTON_NEXT, 0); /* move to slot 1 (Carmen Sandiego) */

    int reset_needed = boot_splash_handle_button(&state, BOOT_SPLASH_BUTTON_SELECT, mock_disk_image_setter);

    CHECK(reset_needed == 1, "test_select_invokes_setter_with_current_slots_reram_address: signals reset needed");
    CHECK(g_setter_call_count == 1, "test_select_invokes_setter_with_current_slots_reram_address: setter called exactly once");

    const uint8_t *expected_ptr = (const uint8_t *)(uintptr_t)cartridge_slots[1].reram_addr;
    CHECK(g_last_selected_image == expected_ptr,
          "test_select_invokes_setter_with_current_slots_reram_address: pointer matches selected slot's ReRAM address");
}

static void test_select_with_null_setter_is_a_safe_noop(void) {
    boot_splash_state_t state;
    boot_splash_init(&state);
    reset_mock_setter();

    int reset_needed = boot_splash_handle_button(&state, BOOT_SPLASH_BUTTON_SELECT, 0);
    CHECK(reset_needed == 0, "test_select_with_null_setter_is_a_safe_noop: no reset signaled");
    CHECK(g_setter_call_count == 0, "test_select_with_null_setter_is_a_safe_noop: setter never called");
}

static void test_none_button_is_a_noop(void) {
    boot_splash_state_t state;
    boot_splash_init(&state);
    reset_mock_setter();

    int reset_needed = boot_splash_handle_button(&state, BOOT_SPLASH_BUTTON_NONE, mock_disk_image_setter);
    CHECK(reset_needed == 0, "test_none_button_is_a_noop: no reset signaled");
    CHECK(state.selected_index == 0, "test_none_button_is_a_noop: index unchanged");
    CHECK(g_setter_call_count == 0, "test_none_button_is_a_noop: setter never called");
}

static void test_select_can_target_real_disk_trap_set_image(void) {
    /* Integration proof: boot_splash_disk_image_setter_fn's signature must
     * be call-compatible with disk_trap_set_image() itself, not just a
     * same-shaped mock -- this is the actual wiring point requested. */
    boot_splash_state_t state;
    boot_splash_init(&state);

    int reset_needed = boot_splash_handle_button(&state, BOOT_SPLASH_BUTTON_SELECT, disk_trap_set_image);
    CHECK(reset_needed == 1,
          "test_select_can_target_real_disk_trap_set_image: real disk_trap_set_image accepted and reset signaled");
}

int main(void) {
    test_init_selects_first_slot();
    test_next_advances_one_slot();
    test_prev_moves_back_one_slot();
    test_next_wraps_past_last_slot();
    test_prev_wraps_before_first_slot();
    test_select_invokes_setter_with_current_slots_reram_address();
    test_select_with_null_setter_is_a_safe_noop();
    test_none_button_is_a_noop();
    test_select_can_target_real_disk_trap_set_image();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
