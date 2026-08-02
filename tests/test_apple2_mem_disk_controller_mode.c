/*
 * tests/test_apple2_mem_disk_controller_mode.c -- RED-first test for the
 * runtime mode switch that gates whether $C0E0-$C0EF routes to
 * disk_trap.c's fast-sector-read shortcut (default, existing behavior)
 * or disk2_controller.c's real Disk II softswitch/nibble emulation
 * (NEXT_STEPS.md Step 7).
 *
 * Architecture decision (2026-08-01, confirmed with Ryan): disk_trap.c
 * and disk2_controller.c both need the IDENTICAL $C0E0-$C0EF address
 * range for slot 6 -- a genuine address-space collision, not two
 * unrelated features living nearby. Resolution: apple2_mem.c gates which
 * one is active via a runtime mode flag
 * (apple2_mem_set_disk_controller_mode()) -- disk_trap.c remains the
 * default for the boot-splash menu's own synthetic bootloaders
 * (checkerboard/hires demos, dos33_sample.dsk), while
 * APPLE2_MEM_DISK_CONTROLLER_DISK2 opts into disk2_controller.c for real
 * disk images. The two are NEVER both wired to $C0E0-$C0EF at once.
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/disk_trap.h"
#include "../src/disk2_controller.h"

static void test_default_mode_is_disk_trap(void) {
    apple2_mem_reset();
    /* Default (no explicit mode switch) must preserve every existing
     * synthetic-bootloader/menu behavior exactly -- disk_trap.c's
     * $C0E0/$C0E1/$C0EC protocol. */
    static uint8_t mock_disk[256];
    memset(mock_disk, 0, sizeof(mock_disk));
    mock_disk[0] = 0xAA;
    disk_trap_set_image(mock_disk);

    write6502(0xC0E0, 0x00); /* track */
    write6502(0xC0E1, 0x00); /* sector -> selects (0,0), offset 0 */
    uint8_t b = read6502(0xC0EC);
    assert(b == 0xAA);
    printf("PASS: test_default_mode_is_disk_trap\n");
}

static void test_disk2_mode_routes_to_disk2_controller(void) {
    apple2_mem_reset();
    apple2_mem_set_disk_controller_mode(APPLE2_MEM_DISK_CONTROLLER_DISK2);

    disk2_controller_t *ctl = apple2_mem_get_disk2_controller();
    assert(ctl != NULL);

    static disk2_nibble_track_t tracks[DISK2_MAX_TRACKS];
    memset(tracks, 0, sizeof(tracks));
    tracks[0].data[0] = 0x55;
    tracks[0].data[1] = 0x66;
    tracks[0].length = 2;
    disk2_controller_load_nibble_disk(ctl, 0, tracks, 0);

    /* Turn the drive on via the real $C0E0-$C0EF bus addresses (not the
     * direct disk2_controller_access() call) -- this proves apple2_mem.c
     * itself is dispatching to disk2_controller.c, not just that
     * disk2_controller.c works standalone (already covered by
     * test_disk2_controller.c). */
    write6502(0xC0E9, 0x00); /* DRIVEON = loc 0x9 -> $C0E9 */
    write6502(0xC0EE, 0x00); /* DRIVEREADMODE = loc 0xE -> $C0EE (Q7 low) */

    uint8_t first = read6502(0xC0EC); /* DRIVEREAD = loc 0xC -> $C0EC */
    assert(first == 0x55); /* First access loads byte 0 (0x55) */
    uint8_t sub = read6502(0xC0EC);
    assert((sub & 0x80) == 0); /* Sub-32-cycle access yields bit 7 = 0 */

    clockticks6502 += 32;
    uint8_t second = read6502(0xC0EC);
    assert(second == 0x66); /* Next access after 32 cycles loads byte 1 (0x66) */

    printf("PASS: test_disk2_mode_routes_to_disk2_controller\n");
}

static void test_switching_back_to_disk_trap_restores_original_behavior(void) {
    apple2_mem_reset();
    apple2_mem_set_disk_controller_mode(APPLE2_MEM_DISK_CONTROLLER_DISK2);
    apple2_mem_set_disk_controller_mode(APPLE2_MEM_DISK_CONTROLLER_DISK_TRAP);

    static uint8_t mock_disk[256];
    memset(mock_disk, 0, sizeof(mock_disk));
    mock_disk[0] = 0x77;
    disk_trap_set_image(mock_disk);

    write6502(0xC0E0, 0x00);
    write6502(0xC0E1, 0x00);
    uint8_t b = read6502(0xC0EC);
    assert(b == 0x77);
    printf("PASS: test_switching_back_to_disk_trap_restores_original_behavior\n");
}

static void test_reset_restores_default_mode(void) {
    /* apple2_mem_reset() must restore DISK_TRAP as the default mode --
     * otherwise a stale DISK2 mode selection from a previous
     * boot/session could silently break the synthetic-bootloader menu
     * path on the next reset, a real regression risk given this
     * session's established "reset must fully restore defaults"
     * precedent (disk_trap_reset(), apple2_mem_clear_button_states(),
     * etc. -- see this file's own reset()'s comment history). */
    apple2_mem_reset();
    apple2_mem_set_disk_controller_mode(APPLE2_MEM_DISK_CONTROLLER_DISK2);
    apple2_mem_reset();

    static uint8_t mock_disk[256];
    memset(mock_disk, 0, sizeof(mock_disk));
    mock_disk[0] = 0x33;
    disk_trap_set_image(mock_disk);

    write6502(0xC0E0, 0x00);
    write6502(0xC0E1, 0x00);
    uint8_t b = read6502(0xC0EC);
    assert(b == 0x33);
    printf("PASS: test_reset_restores_default_mode\n");
}

int main(void) {
    test_default_mode_is_disk_trap();
    test_disk2_mode_routes_to_disk2_controller();
    test_switching_back_to_disk_trap_restores_original_behavior();
    test_reset_restores_default_mode();

    printf("All tests passed.\n");
    return 0;
}
