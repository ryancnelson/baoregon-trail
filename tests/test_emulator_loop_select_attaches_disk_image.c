/*
 * tests/test_emulator_loop_select_attaches_disk_image.c -- integration
 * test proving that selecting a game in the splash menu
 * (baoregon_emulator_poll_input() -> boot_splash's SELECT ->
 * disk_trap_set_image()) actually results in disk_trap.c reporting the
 * new image via disk_trap_get_image_ptr(), matching the selected
 * cartridge slot's reram_addr -- something only ever exercised through
 * disk_trap.c's own unit tests or a mock setter callback
 * (tests/test_boot_splash*.c), never through the real end-to-end
 * emulator_loop -> boot_splash -> disk_trap_set_image() wiring with the
 * real disk_trap_get_image_ptr() getter.
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

    /* Before selecting anything, no disk image should be registered. */
    if (disk_trap_get_image_ptr() != 0) {
        fprintf(stderr, "FAIL: disk_trap_get_image_ptr() is non-NULL before "
                        "any game has been selected\n");
        assert(0);
    }
    printf("PASS: no disk image registered before selection\n");

    /* Select slot 0 (default highlighted slot) via SELECT (PB2) edge. */
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(2, 0);
    baoregon_emulator_poll_input();

    assert(baoregon_emulator_is_in_splash_menu() == 0);

    const uint8_t *image_ptr = disk_trap_get_image_ptr();
    uint32_t expected_addr = cartridge_slots[0].reram_addr;
    uint32_t actual_addr = (uint32_t)(uintptr_t)image_ptr;

    if (actual_addr != expected_addr) {
        fprintf(stderr, "FAIL: disk_trap_get_image_ptr() returned 0x%08X, "
                        "expected cartridge_slots[0].reram_addr 0x%08X -- "
                        "the boot_splash SELECT -> disk_trap_set_image() "
                        "wiring didn't actually attach the selected slot's "
                        "address\n", actual_addr, expected_addr);
        assert(0);
    }
    printf("PASS: disk_trap_get_image_ptr() matches cartridge_slots[0].reram_addr "
           "after SELECT\n");

    printf("All tests passed.\n");
    return 0;
}
