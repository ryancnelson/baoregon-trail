/*
 * tests/test_disk2_controller.c -- unit test for disk2_controller.c/.h,
 * ported from whscullin/apple2js's disk2.ts + NibbleDiskDriver.ts (MIT
 * license -- see src/disk2_controller.c for full attribution).
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../src/disk2_controller.h"
#include "../src/cpu6502.h"

/* Softswitch offsets, matching disk2_controller.c's internal enum
 * (not exposed in the header, so redefined here identically for test
 * readability -- kept in sync manually since the header only documents
 * "0x00-0x0F", not named constants). */
#define LOC_PHASE0OFF 0x0
#define LOC_PHASE0ON  0x1
#define LOC_PHASE1OFF 0x2
#define LOC_PHASE1ON  0x3
#define LOC_PHASE2OFF 0x4
#define LOC_PHASE2ON  0x5
#define LOC_PHASE3OFF 0x6
#define LOC_PHASE3ON  0x7
#define LOC_DRIVEOFF  0x8
#define LOC_DRIVEON   0x9
#define LOC_DRIVE1    0xA
#define LOC_DRIVE2    0xB
#define LOC_DRIVEREAD 0xC
#define LOC_DRIVEWRITE 0xD
#define LOC_DRIVEREADMODE 0xE
#define LOC_DRIVEWRITEMODE 0xF

static void test_reset_defaults(void) {
    disk2_controller_t ctl;
    disk2_controller_reset(&ctl);

    assert(ctl.motor_on == 0);
    assert(ctl.q6 == 0);
    assert(ctl.q7 == 0);
    assert(ctl.selected_drive == 0);
    assert(ctl.latch == 0);
    assert(ctl.drive[0].track == 0);
    assert(ctl.drive[1].track == 0);
    assert(ctl.drive[0].phase == 0);
    assert(ctl.drive[0].head == 0);
    printf("PASS: test_reset_defaults\n");
}

static void test_drive_on_off(void) {
    disk2_controller_t ctl;
    disk2_controller_reset(&ctl);

    disk2_controller_access(&ctl, LOC_DRIVEON, 1, 0);
    assert(ctl.motor_on == 1);

    disk2_controller_access(&ctl, LOC_DRIVEOFF, 1, 0);
    assert(ctl.motor_on == 0);
    printf("PASS: test_drive_on_off\n");
}

static void test_q6_q7_mode_switches(void) {
    disk2_controller_t ctl;
    disk2_controller_reset(&ctl);

    disk2_controller_access(&ctl, LOC_DRIVEWRITEMODE, 1, 0);
    assert(ctl.q7 == 1);
    disk2_controller_access(&ctl, LOC_DRIVEREADMODE, 1, 0);
    assert(ctl.q7 == 0);

    disk2_controller_access(&ctl, LOC_DRIVEWRITE, 1, 0);
    assert(ctl.q6 == 1);
    disk2_controller_access(&ctl, LOC_DRIVEREAD, 1, 0);
    assert(ctl.q6 == 0);
    printf("PASS: test_q6_q7_mode_switches\n");
}

static void test_phase_stepping_ignored_while_drive_off(void) {
    disk2_controller_t ctl;
    disk2_controller_reset(&ctl);

    /* Per Sather UtA2e p.9-12 and disk2.ts's setPhase(): phase changes do
     * nothing while the drive motor is off. */
    disk2_controller_access(&ctl, LOC_PHASE1ON, 1, 0);
    assert(ctl.drive[0].track == 0);
    assert(ctl.drive[0].phase == 0);
    printf("PASS: test_phase_stepping_ignored_while_drive_off\n");
}

static void test_phase_stepping_moves_track_when_drive_on(void) {
    disk2_controller_t ctl;
    disk2_controller_reset(&ctl);
    disk2_controller_access(&ctl, LOC_DRIVEON, 1, 0);

    /* From phase 0, engaging phase 1 steps forward one whole track (2
     * quarter-tracks) -- PHASE_DELTA[0][1] == 1, *2 == 2 quarter-tracks. */
    disk2_controller_access(&ctl, LOC_PHASE1ON, 1, 0);
    assert(ctl.drive[0].track == 2);
    assert(ctl.drive[0].phase == 1);

    /* Step back down: PHASE_DELTA[1][0] == -1, *2 == -2 -> back to 0. */
    disk2_controller_access(&ctl, LOC_PHASE0ON, 1, 0);
    assert(ctl.drive[0].track == 0);
    assert(ctl.drive[0].phase == 0);
    printf("PASS: test_phase_stepping_moves_track_when_drive_on\n");
}

static void test_track_clamps_at_zero(void) {
    disk2_controller_t ctl;
    disk2_controller_reset(&ctl);
    disk2_controller_access(&ctl, LOC_DRIVEON, 1, 0);

    /* Stepping to phase 3 from phase 0 (PHASE_DELTA[0][3] == -1, *2 ==
     * -2) must clamp to 0, not go negative. */
    disk2_controller_access(&ctl, LOC_PHASE3ON, 1, 0);
    assert(ctl.drive[0].track == 0);
    printf("PASS: test_track_clamps_at_zero\n");
}

static void test_drive_select(void) {
    disk2_controller_t ctl;
    disk2_controller_reset(&ctl);

    disk2_controller_access(&ctl, LOC_DRIVE2, 1, 0);
    assert(ctl.selected_drive == 1);

    disk2_controller_access(&ctl, LOC_DRIVE1, 1, 0);
    assert(ctl.selected_drive == 0);
    printf("PASS: test_drive_select\n");
}

/* Tests cycle-accurate shift-register timing (32 CPU cycles per nibble byte)
 * and bit-7 latch clearing on read access. */
static void test_nibble_read_timing(void) {
    disk2_controller_t ctl;
    disk2_controller_reset(&ctl);

    static disk2_nibble_track_t tracks[DISK2_MAX_TRACKS];
    memset(tracks, 0, sizeof(tracks));
    tracks[0].data[0] = 0xAA;
    tracks[0].data[1] = 0xBB;
    tracks[0].length = 2;
    disk2_controller_load_nibble_disk(&ctl, 0, tracks, 0);

    disk2_controller_access(&ctl, LOC_DRIVEON, 1, 0);
    disk2_controller_access(&ctl, LOC_DRIVEREADMODE, 1, 0);

    /* First access loads byte 0 (0xAA) into latch and clears bit 7 (0x2A). */
    uint8_t first = disk2_controller_access(&ctl, LOC_DRIVEREAD, 0, 0);
    assert(first == 0xAA);

    /* Subsequent access before 32 cycles pass yields bit 7 = 0 (0x2A). */
    uint8_t sub = disk2_controller_access(&ctl, LOC_DRIVEREAD, 0, 0);
    assert((sub & 0x80) == 0);

    /* Advance clockticks6502 by 32 cycles. */
    clockticks6502 += 32;

    /* Next access after 32 cycles loads byte 1 (0xBB). */
    uint8_t second = disk2_controller_access(&ctl, LOC_DRIVEREAD, 0, 0);
    assert(second == 0xBB);

    printf("PASS: test_nibble_read_timing\n");
}

static void test_nibble_read_no_disk_returns_zero(void) {
    disk2_controller_t ctl;
    disk2_controller_reset(&ctl);
    disk2_controller_access(&ctl, LOC_DRIVEON, 1, 0);
    disk2_controller_access(&ctl, LOC_DRIVEREADMODE, 1, 0);

    /* No disk2_controller_load_nibble_disk() call -- has_disk stays 0. */
    uint8_t r = disk2_controller_access(&ctl, LOC_DRIVEREAD, 0, 0);
    assert(r == 0);
    printf("PASS: test_nibble_read_no_disk_returns_zero\n");
}

static void test_nibble_read_off_when_drive_off(void) {
    disk2_controller_t ctl;
    disk2_controller_reset(&ctl);

    static disk2_nibble_track_t tracks[DISK2_MAX_TRACKS];
    memset(tracks, 0, sizeof(tracks));
    tracks[0].data[0] = 0x99;
    tracks[0].length = 1;
    disk2_controller_load_nibble_disk(&ctl, 0, tracks, 0);

    /* Drive left off (never sent LOC_DRIVEON) -- must produce latch=0
     * unconditionally, matching NibbleDiskDriver.onQ6Low()'s isOn()
     * gate, never real track data. */
    uint8_t r = disk2_controller_access(&ctl, LOC_DRIVEREAD, 0, 0);
    assert(r == 0);
    printf("PASS: test_nibble_read_off_when_drive_off\n");
}

int main(void) {
    test_reset_defaults();
    test_drive_on_off();
    test_q6_q7_mode_switches();
    test_phase_stepping_ignored_while_drive_off();
    test_phase_stepping_moves_track_when_drive_on();
    test_track_clamps_at_zero();
    test_drive_select();
    test_nibble_read_timing();
    test_nibble_read_no_disk_returns_zero();
    test_nibble_read_off_when_drive_off();

    printf("All tests passed.\n");
    return 0;
}
