/*
 * tests/test_disk_trap_get_selected_offset_rejects_invalid.c -- regression
 * test proving disk_trap_get_selected_offset() is left UNCHANGED when
 * disk_trap_select_sector() rejects an invalid (track, sector) -- matching
 * disk_trap.h's documented contract ("on error, the previously selected
 * sector (if any) is left unchanged") but previously only verified via
 * disk_trap_read_byte()'s behavior (test_apple2_mem.c's
 * test_disk_trap_invalid_track_select_does_not_disturb_prior_selection),
 * never directly against disk_trap_get_selected_offset() itself.
 */
#include <assert.h>
#include <stdio.h>
#include "../src/disk_trap.h"

int main(void) {
    disk_trap_reset();

    /* Valid select: track 1, sector 0 -> offset 4096. */
    assert(disk_trap_select_sector(1, 0) == 0);
    assert(disk_trap_get_selected_offset() == 4096u);
    assert(disk_trap_has_selection() == 1);

    /* Invalid select (track out of range) must be rejected AND leave
     * the offset/selection state completely untouched. */
    int rc = disk_trap_select_sector(99, 99);
    assert(rc == -1);
    assert(disk_trap_get_selected_offset() == 4096u);
    assert(disk_trap_has_selection() == 1);

    /* Invalid select (sector out of range, track in range) -- same
     * guarantee. */
    rc = disk_trap_select_sector(2, 99);
    assert(rc == -1);
    assert(disk_trap_get_selected_offset() == 4096u);
    assert(disk_trap_has_selection() == 1);

    printf("PASS: test_disk_trap_get_selected_offset_rejects_invalid\n");
    printf("All tests passed.\n");
    return 0;
}
