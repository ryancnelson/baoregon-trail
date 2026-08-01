/*
 * tests/test_emulator_loop_select_slot1_attaches_disk_image.c -- integration
 * test proving that navigating NEXT (PB1) then pressing SELECT (PB2) in splash menu
 * attaches cartridge_slots[1].reram_addr to disk_trap.c.
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

    /* Press NEXT (PB1) to move highlight to slot 1 */
    apple2_mem_set_button_state(1, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(1, 0);
    baoregon_emulator_poll_input();

    assert(baoregon_emulator_get_current_slot() == &cartridge_slots[1]);

    /* Press SELECT (PB2) edge to select slot 1 */
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(2, 0);
    baoregon_emulator_poll_input();

    assert(baoregon_emulator_is_in_splash_menu() == 0);

    const uint8_t *image_ptr = disk_trap_get_image_ptr();
    uint32_t expected_addr = cartridge_slots[1].reram_addr;
    uint32_t actual_addr = (uint32_t)(uintptr_t)image_ptr;

    if (actual_addr != expected_addr) {
        fprintf(stderr, "FAIL: disk_trap_get_image_ptr() returned 0x%08X, "
                        "expected cartridge_slots[1].reram_addr 0x%08X\n",
                        actual_addr, expected_addr);
        assert(0);
    }
    printf("PASS: disk_trap_get_image_ptr() matches cartridge_slots[1].reram_addr "
           "after NEXT + SELECT\n");

    printf("All tests passed.\n");
    return 0;
}
