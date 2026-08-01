/*
 * RED test: boot_splash_poll_apple2_mem_buttons() -- edge-triggered
 * bridge from real apple2_mem.c pushbutton state (PB0/PB1/PB2,
 * apple2_mem_get_button_state()) to boot_splash's PREV/NEXT/SELECT
 * cartridge selector. Real badge buttons are level-based; this proves
 * a button held across multiple polls fires exactly once (on the
 * released->pressed edge), not once per poll.
 */
#include <stdio.h>
#include "../src/boot_splash.h"
#include "../src/apple2_mem.h"
#include "../src/cartridge_layout.h"

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

static void test_pressing_pb1_advances_selection_via_real_apple2_mem_state(void) {
    apple2_mem_reset();
    boot_splash_state_t state;
    boot_splash_init(&state);
    boot_splash_button_edge_state_t edge_state;
    boot_splash_button_edge_state_init(&edge_state);

    apple2_mem_set_button_state(1, 1); /* PB1 = NEXT, physically pressed */
    int reset_needed = boot_splash_poll_apple2_mem_buttons(&state, &edge_state, 0);

    CHECK(reset_needed == 0, "test_pressing_pb1_advances_selection_via_real_apple2_mem_state: no reset yet");
    CHECK(state.selected_index == 1,
          "test_pressing_pb1_advances_selection_via_real_apple2_mem_state: NEXT advanced selection");
}

static void test_holding_button_across_multiple_polls_fires_only_once(void) {
    apple2_mem_reset();
    boot_splash_state_t state;
    boot_splash_init(&state);
    boot_splash_button_edge_state_t edge_state;
    boot_splash_button_edge_state_init(&edge_state);

    apple2_mem_set_button_state(1, 1); /* PB1 held down */
    boot_splash_poll_apple2_mem_buttons(&state, &edge_state, 0); /* poll 1: fires, index -> 1 */
    boot_splash_poll_apple2_mem_buttons(&state, &edge_state, 0); /* poll 2: still held, must NOT re-fire */
    boot_splash_poll_apple2_mem_buttons(&state, &edge_state, 0); /* poll 3: still held, must NOT re-fire */

    CHECK(state.selected_index == 1,
          "test_holding_button_across_multiple_polls_fires_only_once: index advanced exactly once, not three times");
}

static void test_release_then_press_again_fires_a_second_edge(void) {
    apple2_mem_reset();
    boot_splash_state_t state;
    boot_splash_init(&state);
    boot_splash_button_edge_state_t edge_state;
    boot_splash_button_edge_state_init(&edge_state);

    apple2_mem_set_button_state(1, 1);
    boot_splash_poll_apple2_mem_buttons(&state, &edge_state, 0); /* index -> 1 */

    apple2_mem_set_button_state(1, 0); /* released */
    boot_splash_poll_apple2_mem_buttons(&state, &edge_state, 0); /* no-op, still released */

    apple2_mem_set_button_state(1, 1); /* pressed again -- new edge */
    boot_splash_poll_apple2_mem_buttons(&state, &edge_state, 0); /* index -> 2 */

    CHECK(state.selected_index == 2,
          "test_release_then_press_again_fires_a_second_edge");
}

static void test_pb0_maps_to_prev(void) {
    apple2_mem_reset();
    boot_splash_state_t state;
    boot_splash_init(&state); /* starts at index 0 */
    boot_splash_button_edge_state_t edge_state;
    boot_splash_button_edge_state_init(&edge_state);

    apple2_mem_set_button_state(0, 1); /* PB0 = PREV */
    boot_splash_poll_apple2_mem_buttons(&state, &edge_state, 0);

    CHECK(state.selected_index == CARTRIDGE_SLOT_COUNT - 1,
          "test_pb0_maps_to_prev: PREV from index 0 wraps to last slot");
}

static void test_pb2_maps_to_select_and_invokes_setter(void) {
    apple2_mem_reset();
    boot_splash_state_t state;
    boot_splash_init(&state);
    boot_splash_button_edge_state_t edge_state;
    boot_splash_button_edge_state_init(&edge_state);
    reset_mock_setter();

    apple2_mem_set_button_state(2, 1); /* PB2 = SELECT */
    int reset_needed = boot_splash_poll_apple2_mem_buttons(&state, &edge_state, mock_disk_image_setter);

    CHECK(reset_needed == 1, "test_pb2_maps_to_select_and_invokes_setter: reset signaled");
    CHECK(g_setter_call_count == 1, "test_pb2_maps_to_select_and_invokes_setter: setter called once");

    const uint8_t *expected_ptr = (const uint8_t *)(uintptr_t)cartridge_slots[0].reram_addr;
    CHECK(g_last_selected_image == expected_ptr,
          "test_pb2_maps_to_select_and_invokes_setter: correct slot address passed");
}

static void test_no_buttons_pressed_is_a_noop(void) {
    apple2_mem_reset();
    boot_splash_state_t state;
    boot_splash_init(&state);
    boot_splash_button_edge_state_t edge_state;
    boot_splash_button_edge_state_init(&edge_state);

    int reset_needed = boot_splash_poll_apple2_mem_buttons(&state, &edge_state, 0);
    CHECK(reset_needed == 0, "test_no_buttons_pressed_is_a_noop: no reset");
    CHECK(state.selected_index == 0, "test_no_buttons_pressed_is_a_noop: index unchanged");
}

int main(void) {
    test_pressing_pb1_advances_selection_via_real_apple2_mem_state();
    test_holding_button_across_multiple_polls_fires_only_once();
    test_release_then_press_again_fires_a_second_edge();
    test_pb0_maps_to_prev();
    test_pb2_maps_to_select_and_invokes_setter();
    test_no_buttons_pressed_is_a_noop();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
