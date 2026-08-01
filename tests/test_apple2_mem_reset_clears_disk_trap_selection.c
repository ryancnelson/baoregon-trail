/*
 * tests/test_apple2_mem_reset_clears_disk_trap_selection.c -- regression
 * lock for the real bug fixed by wiring disk_trap_reset() into
 * apple2_mem_reset() (src/apple2_mem.c, commit 4600ff9): before that fix,
 * a sector selected via $C0E0/$C0E1 before a reset (e.g. the 3-button
 * soft-reset combo, or a failed boot retry) survived apple2_mem_reset()
 * unchanged in disk_trap.c's own g_have_selection/g_selected_sector_offset
 * state -- only apple2_mem.c's local streaming cursor
 * (g_disk_stream_cursor/g_pending_track) was zeroed. A post-reset $C0EC
 * read with no new $C0E0/$C0E1 selection would silently resume streaming
 * the STALE previously-selected sector's data instead of behaving like
 * "no selection yet" (matching tests/test_disk_trap_safe_defaults.c's
 * documented 0x00 safe-default contract).
 *
 * This is the missing end-to-end proof that disk_trap.h's own doc
 * comment for disk_trap_reset() referenced by filename but which was
 * never actually created -- disk_trap_has_selection()/
 * disk_trap_get_selected_offset() got their own unit tests, but nothing
 * exercised the actual apple2_mem_reset() -> disk_trap_reset() wiring
 * through the real $C0E0/$C0E1/$C0EC softswitch path end-to-end.
 */
#include <assert.h>
#include <stdio.h>
#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/disk_trap.h"

int main(void) {
    static uint8_t disk_image[35 * 16 * 256];
    for (int i = 0; i < (int)sizeof(disk_image); i++) {
        disk_image[i] = 0x00;
    }
    disk_image[0] = 0xAA;
    disk_image[1] = 0xBB;
    disk_image[2] = 0xCC;
    disk_image[3] = 0xDD;

    apple2_mem_reset();
    apple2_mem_set_disk_image(disk_image);

    /* Select track 0, sector 0 via the real softswitch path. */
    write6502(0xC0E0, 0x00);
    write6502(0xC0E1, 0x00);

    uint8_t first_byte = read6502(0xC0EC);
    if (first_byte != 0xAA) {
        fprintf(stderr, "FAIL: test setup didn't actually select the sector "
                        "(expected 0xAA, got 0x%02X)\n", first_byte);
        assert(0);
    }
    if (!disk_trap_has_selection()) {
        fprintf(stderr, "FAIL: test setup -- disk_trap_has_selection() "
                        "should report 1 after a successful select\n");
        assert(0);
    }
    printf("PASS: setup -- sector selected, first byte reads 0xAA, "
           "has_selection() reports 1\n");

    /* Reset without re-selecting or re-attaching the disk image --
     * exactly like the 3-button-combo reset or a failed-boot retry. */
    apple2_mem_reset();

    if (disk_trap_has_selection()) {
        fprintf(stderr, "FAIL: disk_trap_has_selection() still reports 1 "
                        "after apple2_mem_reset() -- stale selection state "
                        "survived the reset\n");
        assert(0);
    }
    printf("PASS: disk_trap_has_selection() reports 0 after reset\n");

    /* Immediately read $C0EC with no new selection since the reset --
     * must be the safe 0x00 default, not the stale sector's stream. */
    uint8_t byte_after_reset = read6502(0xC0EC);
    if (byte_after_reset != 0x00) {
        fprintf(stderr, "FAIL: apple2_mem_reset() left a stale disk sector "
                        "selection active -- $C0EC read returned 0x%02X "
                        "instead of the safe 0x00 default\n", byte_after_reset);
        assert(0);
    }
    printf("PASS: test_apple2_mem_reset_clears_disk_trap_selection\n");
    printf("All tests passed.\n");
    return 0;
}
