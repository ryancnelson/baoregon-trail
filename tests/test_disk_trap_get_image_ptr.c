/*
 * tests/test_disk_trap_get_image_ptr.c -- Unit test for disk_trap_get_image_ptr.
 */
#include <assert.h>
#include <stdio.h>
#include "disk_trap.h"

static const uint8_t g_dummy_dsk[143360] = {0x01, 0x02, 0x03};

static void test_disk_trap_get_image_ptr(void) {
    disk_trap_clear_image();

    /* Before mounting any disk, getter returns NULL */
    assert(disk_trap_get_image_ptr() == (const uint8_t *)0);

    /* Mount image and verify getter returns image pointer */
    disk_trap_set_image(g_dummy_dsk);
    assert(disk_trap_get_image_ptr() == g_dummy_dsk);
    assert(disk_trap_get_image_ptr()[0] == 0x01);

    /* Clear image and verify getter returns NULL */
    disk_trap_clear_image();
    assert(disk_trap_get_image_ptr() == (const uint8_t *)0);

    printf("PASS: test_disk_trap_get_image_ptr\n");
}

int main(void) {
    test_disk_trap_get_image_ptr();
    return 0;
}
