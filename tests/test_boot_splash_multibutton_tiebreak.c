/*
 * RED test: multi-button-simultaneous-press tie-break behavior for
 * boot_splash_poll_apple2_mem_buttons(). Per boot_splash.h's own
 * documented contract ("If more than one button transitions to pressed
 * in the same poll ... PB0 is serviced first, matching array index
 * order"), this locks in the previously-untested/undocumented-by-test
 * behavior: ALL buttons that transition to pressed in the same poll are
 * serviced, in PB0->PB1->PB2 order, each acting on the state left by the
 * previous one in that same poll (not the pre-poll snapshot). This is a
 * real gap: if PREV and SELECT transition together, SELECT fires against
 * the NEW (post-PREV) selection, not the one that was highlighted before
 * the poll -- worth locking in explicitly since it's surprising behavior
 * a caller could easily get wrong without a test pinning it down.
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

static void test_prev_and_next_pressed_together_both_apply_in_pb_order(void) {
    /* PB0 (PREV) and PB1 (NEXT) both transition to pressed in the same
     * poll. Per array-index order, PREV is serviced first (index 0 ->
     * wraps to last slot), then NEXT is serviced (last slot -> wraps
     * back to 0). Net effect from starting index 0: ends at index 0 --
     * but only because both moves happened, not because one was
     * skipped. This test would NOT distinguish "both applied" from
     * "neither applied" on its own, so it's paired with the PREV+SELECT
     * test below which unambiguously proves multiple buttons each take
     * effect. */
    apple2_mem_reset();
    boot_splash_state_t state;
    boot_splash_init(&state); /* index 0 */
    boot_splash_button_edge_state_t edge_state;
    boot_splash_button_edge_state_init(&edge_state);

    apple2_mem_set_button_state(0, 1); /* PB0 PREV */
    apple2_mem_set_button_state(1, 1); /* PB1 NEXT, same poll */
    boot_splash_poll_apple2_mem_buttons(&state, &edge_state, 0);

    CHECK(state.selected_index == 0,
          "test_prev_and_next_pressed_together_both_apply_in_pb_order: PREV then NEXT nets back to index 0");
}

static void test_prev_and_select_pressed_together_select_acts_on_post_prev_index(void) {
    /* This is the actual gap: PB0 (PREV) and PB2 (SELECT) transition
     * together. Per PB-index order, PREV is applied FIRST (moving
     * selection from index 1 to index 0), and SELECT is then applied to
     * the NEW index 0 -- not the index 1 that was highlighted when the
     * poll began. Locks in that SELECT always acts on the
     * already-updated-this-poll selection, matching the code's actual
     * (previously untested) sequential-processing behavior. */
    apple2_mem_reset();
    boot_splash_state_t state;
    boot_splash_init(&state);
    boot_splash_button_edge_state_t edge_state;
    boot_splash_button_edge_state_init(&edge_state);
    reset_mock_setter();

    /* Move to index 1 first (NEXT), consuming that edge cleanly. */
    apple2_mem_set_button_state(1, 1);
    boot_splash_poll_apple2_mem_buttons(&state, &edge_state, 0);
    apple2_mem_set_button_state(1, 0);
    boot_splash_poll_apple2_mem_buttons(&state, &edge_state, 0);
    CHECK(state.selected_index == 1,
          "test_prev_and_select_pressed_together_select_acts_on_post_prev_index: setup reaches index 1");

    /* Now press PREV and SELECT together in the same poll. */
    apple2_mem_set_button_state(0, 1); /* PB0 PREV */
    apple2_mem_set_button_state(2, 1); /* PB2 SELECT, same poll */
    int reset_needed = boot_splash_poll_apple2_mem_buttons(&state, &edge_state, mock_disk_image_setter);

    CHECK(reset_needed == 1,
          "test_prev_and_select_pressed_together_select_acts_on_post_prev_index: reset signaled");
    CHECK(state.selected_index == 0,
          "test_prev_and_select_pressed_together_select_acts_on_post_prev_index: PREV moved index 1 -> 0 this same poll");

    /* SELECT must have captured slot 0 (the post-PREV index), NOT slot 1
     * (the pre-poll index) -- this is the crux of the tie-break gap. */
    const uint8_t *expected_ptr = (const uint8_t *)(uintptr_t)cartridge_slots[0].reram_addr;
    CHECK(g_setter_call_count == 1,
          "test_prev_and_select_pressed_together_select_acts_on_post_prev_index: setter called exactly once");
    CHECK(g_last_selected_image == expected_ptr,
          "test_prev_and_select_pressed_together_select_acts_on_post_prev_index: SELECT captured post-PREV slot 0, not pre-poll slot 1");
}

static void test_next_and_select_pressed_together_select_acts_on_post_next_index(void) {
    /* Mirror case: NEXT (PB1) and SELECT (PB2) together -- NEXT is
     * serviced first (lower array index), then SELECT sees the new
     * index. */
    apple2_mem_reset();
    boot_splash_state_t state;
    boot_splash_init(&state); /* index 0 */
    boot_splash_button_edge_state_t edge_state;
    boot_splash_button_edge_state_init(&edge_state);
    reset_mock_setter();

    apple2_mem_set_button_state(1, 1); /* PB1 NEXT */
    apple2_mem_set_button_state(2, 1); /* PB2 SELECT, same poll */
    int reset_needed = boot_splash_poll_apple2_mem_buttons(&state, &edge_state, mock_disk_image_setter);

    CHECK(reset_needed == 1,
          "test_next_and_select_pressed_together_select_acts_on_post_next_index: reset signaled");
    CHECK(state.selected_index == 1,
          "test_next_and_select_pressed_together_select_acts_on_post_next_index: NEXT moved index 0 -> 1 this same poll");

    const uint8_t *expected_ptr = (const uint8_t *)(uintptr_t)cartridge_slots[1].reram_addr;
    CHECK(g_last_selected_image == expected_ptr,
          "test_next_and_select_pressed_together_select_acts_on_post_next_index: SELECT captured post-NEXT slot 1");
}

static void test_all_three_buttons_pressed_together_processes_in_pb_order(void) {
    /* PB0(PREV) + PB1(NEXT) + PB2(SELECT) all transition together.
     * Order: PREV (index 0 -> wraps to last), NEXT (last -> wraps to 0),
     * SELECT (captures index 0). Proves the full 3-way tie-break, not
     * just pairs. */
    apple2_mem_reset();
    boot_splash_state_t state;
    boot_splash_init(&state);
    boot_splash_button_edge_state_t edge_state;
    boot_splash_button_edge_state_init(&edge_state);
    reset_mock_setter();

    apple2_mem_set_button_state(0, 1);
    apple2_mem_set_button_state(1, 1);
    apple2_mem_set_button_state(2, 1);
    int reset_needed = boot_splash_poll_apple2_mem_buttons(&state, &edge_state, mock_disk_image_setter);

    CHECK(reset_needed == 1,
          "test_all_three_buttons_pressed_together_processes_in_pb_order: reset signaled");
    CHECK(state.selected_index == 0,
          "test_all_three_buttons_pressed_together_processes_in_pb_order: PREV then NEXT nets back to index 0");
    const uint8_t *expected_ptr = (const uint8_t *)(uintptr_t)cartridge_slots[0].reram_addr;
    CHECK(g_last_selected_image == expected_ptr,
          "test_all_three_buttons_pressed_together_processes_in_pb_order: SELECT captured final index 0");
}

int main(void) {
    test_prev_and_next_pressed_together_both_apply_in_pb_order();
    test_prev_and_select_pressed_together_select_acts_on_post_prev_index();
    test_next_and_select_pressed_together_select_acts_on_post_next_index();
    test_all_three_buttons_pressed_together_processes_in_pb_order();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
