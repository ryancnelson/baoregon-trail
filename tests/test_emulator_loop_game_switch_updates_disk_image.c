/*
 * tests/test_emulator_loop_game_switch_updates_disk_image.c -- integration
 * test proving the actual game-switching scenario: play slot 0, return to
 * splash via the 3-button-combo reset, select a DIFFERENT slot (slot 1),
 * and confirm disk_trap.c's attached image pointer actually updates to
 * the new slot -- not stuck on the previously-selected slot 0.
 *
 * Prior integration tests (test_emulator_loop_select_attaches_disk_image.c,
 * test_emulator_loop_select_slot1_attaches_disk_image.c) only ever
 * selected a slot ONCE from a freshly-initialized emulator -- neither
 * proves the disk_trap image pointer is correctly RE-attached when a
 * user plays one game, resets, and picks a different one, which is the
 * actual real-world "switch games" flow the boot-splash menu exists for.
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "../src/emulator_loop.h"
#include "../src/apple2_mem.h"
#include "../src/disk_trap.h"
#include "../src/cartridge_layout.h"

int main(void) {
    baoregon_emulator_init();

    /* Select slot 0 (default highlighted). */
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(2, 0);
    baoregon_emulator_poll_input();
    assert(baoregon_emulator_is_in_splash_menu() == 0);

    uint32_t slot0_addr = (uint32_t)(uintptr_t)disk_trap_get_image_ptr();
    if (slot0_addr != cartridge_slots[0].reram_addr) {
        fprintf(stderr, "FAIL: setup -- slot 0 not attached correctly\n");
        assert(0);
    }
    printf("PASS: setup -- slot 0 attached (0x%08X)\n", slot0_addr);

    /* Return to splash via 3-button-combo reset (buttons released from
     * the prior SELECT first, matching real hardware -- can't hold all
     * 3 while SELECT's edge is still active in the same poll). */
    apple2_mem_set_button_state(0, 1);
    apple2_mem_set_button_state(1, 1);
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    assert(baoregon_emulator_is_in_splash_menu() == 1);
    apple2_mem_set_button_state(0, 0);
    apple2_mem_set_button_state(1, 0);
    apple2_mem_set_button_state(2, 0);
    /* One more poll with all released to consume the "held through
     * reset" edge-state seeded by reset_to_splash_menu() (see
     * commit 16a7823's spurious-nav fix) -- ensures a subsequent fresh
     * press of NEXT generates a real release->press edge, not a no-op
     * against a still-considered-held button. */
    baoregon_emulator_poll_input();

    /* Now navigate to slot 1 and select it -- the actual "switch games"
     * flow. */
    apple2_mem_set_button_state(1, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(1, 0);
    baoregon_emulator_poll_input();
    assert(baoregon_emulator_get_current_slot() == &cartridge_slots[1]);

    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(2, 0);
    baoregon_emulator_poll_input();
    assert(baoregon_emulator_is_in_splash_menu() == 0);

    uint32_t slot1_addr = (uint32_t)(uintptr_t)disk_trap_get_image_ptr();
    if (slot1_addr != cartridge_slots[1].reram_addr) {
        fprintf(stderr, "FAIL: after switching games, disk_trap_get_image_ptr() "
                        "returned 0x%08X, expected cartridge_slots[1].reram_addr "
                        "0x%08X -- the image pointer did not actually update to "
                        "the newly-selected slot (still stuck on the previous "
                        "game, or some other stale value)\n", slot1_addr,
                        cartridge_slots[1].reram_addr);
        assert(0);
    }
    if (slot1_addr == slot0_addr) {
        fprintf(stderr, "FAIL: slot1_addr equals slot0_addr (0x%08X) -- test "
                        "setup issue, cartridge_slots[0] and [1] should have "
                        "different addresses\n", slot0_addr);
        assert(0);
    }
    printf("PASS: disk_trap_get_image_ptr() correctly updated from slot 0 "
           "(0x%08X) to slot 1 (0x%08X) after switching games\n",
           slot0_addr, slot1_addr);

    printf("All tests passed.\n");
    return 0;
}
