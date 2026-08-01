/*
 * tests/test_disk_trap_clear_image.c -- Unit test for disk_trap_clear_image.
 */
#include <assert.h>
#include <stdio.h>
#include "disk_trap.h"

static const uint8_t g_dummy_dsk[143360] = {0};

static void test_disk_trap_clear_image(void) {
    disk_trap_reset();
    disk_trap_set_image(g_dummy_dsk);
    assert(disk_trap_select_sector(0, 0) == 0);
    assert(disk_trap_has_selection() == 1);

    /* Perform disk_trap_clear_image() */
    disk_trap_clear_image();

    /* Assert image is cleared and selection is reset */
    assert(disk_trap_has_selection() == 0);
    assert(disk_trap_get_selected_offset() == 0u);
    assert(disk_trap_read_byte(0) == 0x00);

    printf("PASS: test_disk_trap_clear_image\n");
}

int main(void) {
    test_disk_trap_clear_image();
    return 0;
}
