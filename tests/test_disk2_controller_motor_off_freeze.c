/*
 * tests/test_disk2_controller_motor_off_freeze.c -- RED-first test for a
 * real bug found while debugging fable-5's Zork I real-disk retry-loop
 * finding (dd86358): disk2_controller.c's nibble_shift() computes
 * elapsed cycles as `clockticks6502 - d->last_cycles` unconditionally
 * whenever the motor is ON, but does NOT freeze/reset d->last_cycles
 * when the motor turns OFF (LOC_DRIVEOFF only sets ctl->motor_on = 0,
 * see disk2_controller_access()'s LOC_DRIVEOFF case).
 *
 * Consequence: if the motor is off for N cycles (RWTS commonly does
 * this between seek/step operations -- confirmed via emu_trace: a real
 * motor off->on transition was observed mid-Zork-I-boot at cycle
 * 1059050->1059152, a 102-cycle gap), the NEXT read after motor-on
 * treats that entire dead time as real elapsed disk-rotation time and
 * jumps the head forward by (gap / 32) nibbles it never should have
 * shifted -- a spurious, silent desync from where RWTS/the boot code
 * believes the head actually is. This is exactly the kind of "almost
 * right, byte position off by a few" symptom that would make an
 * address-field search intermittently miss its sync bytes and retry
 * forever, matching the Zork I loop's cycling pattern.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../src/disk2_controller.h"
#include "../src/cpu6502.h"

#define LOC_DRIVEON 0x9
#define LOC_DRIVEOFF 0x8
#define LOC_DRIVEREADMODE 0xE
#define LOC_DRIVEREAD 0xC

static void test_motor_off_does_not_advance_head_via_dead_time(void) {
    disk2_controller_t ctl;
    disk2_controller_reset(&ctl);

    static disk2_nibble_track_t tracks[DISK2_MAX_TRACKS];
    memset(tracks, 0, sizeof(tracks));
    /* Track intentionally longer than the dead-time nibble count below,
     * so an incorrect head-jump lands on a clearly-different, unrelated
     * byte rather than accidentally wrapping back to the same value
     * (which a 10-byte/10-nibble-gap track would do, masking the bug). */
    for (int i = 0; i < 100; i++) {
        tracks[0].data[i] = (uint8_t)(0x10 + i);
    }
    tracks[0].length = 100;
    disk2_controller_load_nibble_disk(&ctl, 0, tracks, 0);

    clockticks6502 = 0;
    disk2_controller_access(&ctl, LOC_DRIVEON, 1, 0);
    disk2_controller_access(&ctl, LOC_DRIVEREADMODE, 1, 0);

    /* First read: latches head=0's byte (0x10), uninitialized-last_cycles
     * path always shifts on first access. */
    uint8_t first = disk2_controller_access(&ctl, LOC_DRIVEREAD, 0, 0);
    assert(first == 0x10);

    /* Turn the motor OFF, then let a large number of CPU cycles pass
     * (simulating RWTS doing other work -- seeking, verifying, etc --
     * with the drive motor stopped) before turning it back ON. */
    disk2_controller_access(&ctl, LOC_DRIVEOFF, 1, 0);
    clockticks6502 += 10 * 32; /* 10 nibbles' worth of "dead" cycles */
    disk2_controller_access(&ctl, LOC_DRIVEON, 1, 0);

    /* Real Disk II hardware: while the motor is off, the disk isn't
     * spinning, so no new nibbles rotate under the head -- resuming
     * should behave like a fresh restart at the SAME head position
     * (byte 0x10 again), NOT jump forward by (dead_cycles / 32) nibbles
     * as if the disk had kept spinning the whole time. Before the fix,
     * this read incorrectly jumps to head=(0+10)%100=10 (byte 0x1A) --
     * a real, silent desync from where the head should physically be. */
    uint8_t second = disk2_controller_access(&ctl, LOC_DRIVEREAD, 0, 0);
    if (second != 0x10) {
        fprintf(stderr, "FAIL: after motor off->on with 10 nibbles' worth of dead cycles elapsed, "
                        "expected byte 0x10 (head unchanged -- motor-off freeze means no real disk "
                        "rotation happened), got 0x%02X -- motor-off dead time was incorrectly counted "
                        "as real rotation, silently desyncing the head position\n", second);
        assert(0);
    }
    printf("PASS: test_motor_off_does_not_advance_head_via_dead_time\n");
}

int main(void) {
    test_motor_off_does_not_advance_head_via_dead_time();
    printf("All tests passed.\n");
    return 0;
}
