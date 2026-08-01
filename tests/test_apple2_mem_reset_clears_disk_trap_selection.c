/*
 * RED test: does apple2_mem_reset() clear a previously-selected disk
 * sector in disk_trap.c's internal state (g_have_selection /
 * g_selected_sector_offset), or only its own local streaming cursor
 * (g_disk_stream_cursor/g_pending_track)?
 *
 * Hypothesis: apple2_mem_reset() resets g_disk_stream_cursor to 0 and
 * g_pending_track to 0, but does NOT call anything that resets
 * disk_trap.c's own g_have_selection/g_selected_sector_offset (there is
 * no disk_trap_reset() or disk_trap_clear_selection() function, and
 * apple2_mem_reset() doesn't re-select track 0/sector 0 either). If a
 * sector was selected before a 6502-triggered reset ($C0E0/$C0E1 writes
 * from a previous boot attempt or a mid-game soft reset), then
 * immediately after apple2_mem_reset(), reading the disk data port
 * ($C0EC) would return byte 0 of the STALE previously-selected sector,
 * not a "no selection yet" 0x00 default -- because disk_trap.c's
 * g_have_selection is still 1 from before the reset.
 */
#include <assert.h>
#include <stdio.h>
#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/disk_trap.h"

#define DOS33_SECTOR_SIZE 256

int main(void) {
    /* Build a minimal disk image big enough for track 0 sector 0 with
     * the rest zeroed -- only the first few bytes matter for this test. */
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

    /* Select track 0, sector 0 (a real boot attempt would do this). */
    write6502(0xC0E0, 0x00);
    write6502(0xC0E1, 0x00);

    uint8_t first_byte = read6502(0xC0EC);
    if (first_byte != 0xAA) {
        fprintf(stderr, "FAIL: test setup didn't actually select the sector "
                        "(expected 0xAA, got 0x%02X) -- not testing the real "
                        "scenario\n", first_byte);
        assert(0);
    }
    printf("PASS: setup -- sector selected, first byte reads 0xAA\n");

    /* Now reset (simulating a mid-game 3-button-combo reset, or a
     * failed-boot retry) -- WITHOUT re-selecting a sector or
     * re-attaching the disk image (apple2_mem_set_disk_image() is not
     * called again -- exactly like a real reset that doesn't reload
     * the disk). */
    apple2_mem_reset();

    /* Immediately read the disk data port with no new $C0E0/$C0E1
     * selection since the reset. Real expectation: this should behave
     * like "no selection yet" (0x00 default, matching
     * disk_trap_safe_defaults.c's documented contract for an
     * unselected disk trap), NOT silently resume streaming the stale
     * previously-selected sector's bytes from offset 0. */
    uint8_t byte_after_reset = read6502(0xC0EC);
    if (byte_after_reset == 0xAA) {
        fprintf(stderr, "FAIL: apple2_mem_reset() left disk_trap.c's sector "
                        "selection state (g_have_selection/"
                        "g_selected_sector_offset) untouched -- reading "
                        "$C0EC right after reset returned the STALE "
                        "previously-selected sector's byte (0xAA) instead of "
                        "the safe \"no selection yet\" default (0x00)\n");
        assert(0);
    }
    if (byte_after_reset != 0x00) {
        fprintf(stderr, "FAIL: unexpected byte 0x%02X after reset (expected "
                        "0x00 safe default)\n", byte_after_reset);
        assert(0);
    }
    printf("PASS: test_reset_clears_disk_trap_sector_selection\n");
    printf("All tests passed.\n");
    return 0;
}
