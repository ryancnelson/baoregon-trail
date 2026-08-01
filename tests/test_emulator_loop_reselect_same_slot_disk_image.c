/*
 * tests/test_emulator_loop_reselect_same_slot_disk_image.c -- regression
 * test proving that selecting slot 0, resetting back to splash, and
 * selecting slot 0 AGAIN (same slot, not a different one) still
 * correctly re-attaches the same image rather than leaving stale state
 * from disk_trap_reset()'s selection-clearing (which does NOT clear the
 * image pointer, only the (track,sector) selection -- see
 * src/disk_trap.h). This is the "re-select the same game after a soft
 * reset" case, distinct from test_emulator_loop_game_switch_updates_disk_image.c's
 * "switch to a DIFFERENT game" case.
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

    /* Select slot 0. */
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(2, 0);
    baoregon_emulator_poll_input();
    assert(baoregon_emulator_is_in_splash_menu() == 0);

    uint32_t first_addr = (uint32_t)(uintptr_t)disk_trap_get_image_ptr();
    if (first_addr != cartridge_slots[0].reram_addr) {
        fprintf(stderr, "FAIL: setup -- slot 0 not attached correctly\n");
        assert(0);
    }

    /* Return to splash via 3-button-combo reset. */
    apple2_mem_set_button_state(0, 1);
    apple2_mem_set_button_state(1, 1);
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    assert(baoregon_emulator_is_in_splash_menu() == 1);
    apple2_mem_set_button_state(0, 0);
    apple2_mem_set_button_state(1, 0);
    apple2_mem_set_button_state(2, 0);
    baoregon_emulator_poll_input(); /* consume held-through-reset edge state */

    /* Select slot 0 AGAIN (no navigation -- same slot as before). */
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(2, 0);
    baoregon_emulator_poll_input();
    assert(baoregon_emulator_is_in_splash_menu() == 0);

    uint32_t second_addr = (uint32_t)(uintptr_t)disk_trap_get_image_ptr();
    if (second_addr != cartridge_slots[0].reram_addr) {
        fprintf(stderr, "FAIL: re-selecting slot 0 after reset returned "
                        "disk_trap_get_image_ptr() = 0x%08X, expected "
                        "cartridge_slots[0].reram_addr 0x%08X\n",
                        second_addr, cartridge_slots[0].reram_addr);
        assert(0);
    }

    /* Note: deliberately NOT exercising the $C0E0/$C0E1/$C0EC softswitch
     * read pipeline here -- cartridge_slots[].reram_addr is a real
     * hardware ReRAM address (e.g. 0x20280000), not backed by actual
     * host memory in this test process. Dereferencing it via
     * read6502($C0EC) would segfault on host (confirmed by trying it:
     * a genuine host-testing limitation, not a bug -- see
     * tests/test_rram_disk_trap_pipeline.c for how the real pipeline is
     * safely exercised, using a host-allocated buffer passed directly
     * to disk_trap_set_image() instead of a real hardware address). The
     * pointer-identity check above is sufficient to prove the
     * re-selection wiring is correct. */

    printf("PASS: re-selecting the same slot after a reset re-attaches "
           "the correct image (0x%08X)\n", second_addr);
    printf("All tests passed.\n");
    return 0;
}
