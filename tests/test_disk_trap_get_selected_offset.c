/*
 * tests/test_disk_trap_get_selected_offset.c -- Unit test for disk_trap_get_selected_offset.
 */
#include <assert.h>
#include <stdio.h>
#include "disk_trap.h"

static void test_disk_trap_get_selected_offset(void) {
    disk_trap_reset();
    
    /* Before any selection, disk_trap_get_selected_offset() returns 0 */
    assert(disk_trap_get_selected_offset() == 0u);

    /* Select track 0, sector 0 (offset 0) */
    assert(disk_trap_select_sector(0, 0) == 0);
    assert(disk_trap_get_selected_offset() == 0u);

    /* Select track 1, sector 0 (offset 4096 = 0x1000) */
    assert(disk_trap_select_sector(1, 0) == 0);
    assert(disk_trap_get_selected_offset() == 4096u);

    /* Reset zeroes selected offset */
    disk_trap_reset();
    assert(disk_trap_get_selected_offset() == 0u);

    printf("PASS: test_disk_trap_get_selected_offset\n");
}

int main(void) {
    test_disk_trap_get_selected_offset();
    return 0;
}
