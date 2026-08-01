/*
 * tests/test_emulator_loop_select_slot_by_index.c -- unit test for baoregon_emulator_select_slot_by_index.
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../src/emulator_loop.h"
#include "../src/disk_trap.h"

int main(void) {
    baoregon_emulator_init();
    assert(baoregon_emulator_is_in_splash_menu() == 1);

    /* Direct select slot 2 (Karateka) */
    int res = baoregon_emulator_select_slot_by_index(2);
    assert(res == 1);
    assert(baoregon_emulator_is_in_splash_menu() == 0);
    assert(baoregon_emulator_is_game_running() == 1);

    const cartridge_slot_t *slot = baoregon_emulator_get_current_slot();
    assert(slot != NULL);
    assert(strcmp(slot->title, "Karateka") == 0);

    const uint8_t *img = disk_trap_get_image_ptr();
    assert(img != NULL);
    assert((uintptr_t)img == CARTRIDGE_RERAM_BASE + 2 * CARTRIDGE_SLOT_SIZE);

    printf("PASS: baoregon_emulator_select_slot_by_index verified\n");
    printf("All tests passed.\n");
    return 0;
}
