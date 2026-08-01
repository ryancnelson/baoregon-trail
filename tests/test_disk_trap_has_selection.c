/*
 * tests/test_disk_trap_has_selection.c -- Unit test for disk_trap_has_selection.
 */
#include <assert.h>
#include <stdio.h>
#include "disk_trap.h"

static void test_disk_trap_has_selection(void) {
    disk_trap_reset();
    
    /* Before any selection, disk_trap_has_selection() returns 0 */
    assert(disk_trap_has_selection() == 0);

    /* Select track 0, sector 0 */
    assert(disk_trap_select_sector(0, 0) == 0);
    assert(disk_trap_has_selection() == 1);

    /* Reset clears selection */
    disk_trap_reset();
    assert(disk_trap_has_selection() == 0);

    printf("PASS: test_disk_trap_has_selection\n");
}

int main(void) {
    test_disk_trap_has_selection();
    return 0;
}
