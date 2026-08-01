/*
 * tests/test_disk_trap_unmounted_image_safety.c -- unit test verifying
 * disk_trap_read_byte() returns 0x00 safely when image is unmounted or NULL,
 * preventing NULL pointer dereferencing even if selection is active.
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "../src/disk_trap.h"

int main(void) {
    uint8_t dummy_image[DOS33_DISK_IMAGE_SIZE];
    dummy_image[0] = 0xA5;

    /* Mount image and select track 0 sector 0 */
    disk_trap_set_image(dummy_image);
    assert(disk_trap_get_image_ptr() == dummy_image);

    int sel_res = disk_trap_select_sector(0, 0);
    assert(sel_res == 0);
    assert(disk_trap_has_selection() == 1);
    assert(disk_trap_read_byte(0) == 0xA5);

    /* Unmount image via disk_trap_clear_image() */
    disk_trap_clear_image();
    assert(disk_trap_get_image_ptr() == NULL);
    assert(disk_trap_has_selection() == 0);
    assert(disk_trap_read_byte(0) == 0x00);

    /* Explicitly pass NULL to disk_trap_set_image() */
    disk_trap_set_image(NULL);
    assert(disk_trap_get_image_ptr() == NULL);
    assert(disk_trap_read_byte(0) == 0x00);
    assert(disk_trap_read_byte(255) == 0x00);

    printf("PASS: disk_trap unmounted image safety verified\n");
    printf("All tests passed.\n");
    return 0;
}
