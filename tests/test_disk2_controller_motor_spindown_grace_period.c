/*
 * tests/test_disk2_controller_motor_spindown_grace_period.c -- RED-first
 * test for a real bug found retesting Lode Runner's boot with the real
 * apple2-asoft-auto.rom (tools/loderunner_altrom_boot.c, NEXT_STEPS.md):
 * disk2_controller.c turns the disk motor off INSTANTLY on a
 * LOC_DRIVEOFF ($C0E8) access, but real Apple II Disk II hardware does
 * NOT -- accessing $C0E8 merely starts a real ~1-second spindown timer;
 * the drive motor keeps physically spinning (and the data latch keeps
 * producing real shifted-in nibbles) until that timer actually fires.
 * If $C0E9 (DRIVEON) is accessed again before the timer fires, the
 * pending off is cancelled and the motor never stops at all.
 *
 * Confirmed via the real reference implementation this file ports
 * (whscullin/apple2js, js/cards/disk2.ts):
 *
 *     case LOC.DRIVEOFF: // 0x08
 *         if (!this.offTimeout) {
 *             if (state.on) {
 *                 this.offTimeout = window.setTimeout(() => {
 *                     state.on = false;
 *                     ...
 *                 }, 1000);
 *             }
 *         }
 *         break;
 *
 * This file's own header comment already documents dropping this
 * wall-clock timer as "UI polish for a GUI emulator... replaced with an
 * immediate drive-off" -- but this investigation found it is NOT merely
 * cosmetic: real 4am-crack boot code (Lode Runner) relies on the motor
 * staying physically on for a real, non-trivial window after a $C0E8
 * access, polling $C0EC one more time expecting a real shifted-in byte.
 * With an instant motor-off, that poll can never see a fresh nibble and
 * spins forever -- a real, fixable emulator bug, not a cycle-budget
 * issue or a Lode Runner bug.
 *
 * Real Apple II CPU clock is ~1.023 MHz, so a real ~1-second spindown
 * is ~1,023,000 6502 cycles -- this is what's tested here, using the
 * project's own cycle-accurate `clockticks6502` timing model (matching
 * nibble_shift()'s existing cycle-based approach) rather than
 * reintroducing a wall-clock/UI timer dependency.
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

static void test_motor_keeps_spinning_during_real_spindown_grace_period(void) {
    disk2_controller_t ctl;
    disk2_controller_reset(&ctl);

    static disk2_nibble_track_t tracks[DISK2_MAX_TRACKS];
    memset(tracks, 0, sizeof(tracks));
    for (int i = 0; i < 100; i++) {
        tracks[0].data[i] = (uint8_t)(0x90 + i); /* all real bit-7-set GCR-style values */
    }
    tracks[0].length = 100;
    disk2_controller_load_nibble_disk(&ctl, 0, tracks, 0);

    clockticks6502 = 0;
    disk2_controller_access(&ctl, LOC_DRIVEON, 1, 0);
    disk2_controller_access(&ctl, LOC_DRIVEREADMODE, 1, 0);

    /* Real shift while motor is definitely on. */
    uint8_t first = disk2_controller_access(&ctl, LOC_DRIVEREAD, 0, 0);
    if (first != 0x90) {
        fprintf(stderr, "FAIL: expected first real shift to latch 0x90, got 0x%02X\n", first);
        assert(0);
    }

    /* Turn the drive "off" via $C0E8 -- real hardware does NOT stop
     * spinning immediately; it starts a real ~1-second (~1,023,000
     * 6502-cycle) spindown timer. A read shortly after (well within
     * that window, here 1000 cycles later -- far less than a real
     * spindown) should still see the motor as physically spinning and
     * produce a REAL fresh shifted-in nibble (bit 7 set), not a hard
     * zero. */
    disk2_controller_access(&ctl, LOC_DRIVEOFF, 1, 0);
    clockticks6502 += 1000; /* well within the real spindown grace period */
    uint8_t during_grace_period = disk2_controller_access(&ctl, LOC_DRIVEREAD, 0, 0);
    if ((during_grace_period & 0x80) == 0) {
        fprintf(stderr, "FAIL: shortly after a $C0E8 (DRIVEOFF) access, real Apple II hardware's "
                        "drive motor is still physically spinning down (real ~1-second/"
                        "~1,023,000-cycle grace period) -- reading $C0EC should still return a "
                        "real, freshly-shifted nibble with bit 7 set, got 0x%02X (bit 7 clear) "
                        "instead. disk2_controller.c currently turns the motor off INSTANTLY on "
                        "$C0E8, which is exactly the bug causing Lode Runner's real boot code "
                        "(LDA $C0E8 / LDA $C0EC / BPL) to spin forever\n", during_grace_period);
        assert(0);
    }

    printf("PASS: test_motor_keeps_spinning_during_real_spindown_grace_period\n");
}

static void test_motor_actually_stops_after_grace_period_expires(void) {
    disk2_controller_t ctl;
    disk2_controller_reset(&ctl);

    static disk2_nibble_track_t tracks[DISK2_MAX_TRACKS];
    memset(tracks, 0, sizeof(tracks));
    for (int i = 0; i < 100; i++) {
        tracks[0].data[i] = (uint8_t)(0x90 + i);
    }
    tracks[0].length = 100;
    disk2_controller_load_nibble_disk(&ctl, 0, tracks, 0);

    clockticks6502 = 0;
    disk2_controller_access(&ctl, LOC_DRIVEON, 1, 0);
    disk2_controller_access(&ctl, LOC_DRIVEREADMODE, 1, 0);
    disk2_controller_access(&ctl, LOC_DRIVEREAD, 0, 0);

    /* Turn the drive off, then let a real full spindown period's worth
     * of cycles pass (well beyond ~1,023,000) -- the motor should
     * genuinely be off by then, matching real hardware. */
    disk2_controller_access(&ctl, LOC_DRIVEOFF, 1, 0);
    clockticks6502 += 2000000; /* well past the real spindown grace period */
    uint8_t after_real_spindown = disk2_controller_access(&ctl, LOC_DRIVEREAD, 0, 0);
    if ((after_real_spindown & 0x80) != 0) {
        fprintf(stderr, "FAIL: well past the real ~1,023,000-cycle spindown grace period, the "
                        "drive motor should be genuinely off (reading $C0EC should return a hard "
                        "zero, matching this project's existing motor-off behavior), got 0x%02X "
                        "(bit 7 still set) instead -- the spindown grace period fix must still "
                        "let the motor eventually actually turn off, not spin forever\n",
                        after_real_spindown);
        assert(0);
    }

    printf("PASS: test_motor_actually_stops_after_grace_period_expires\n");
}

int main(void) {
    test_motor_keeps_spinning_during_real_spindown_grace_period();
    test_motor_actually_stops_after_grace_period_expires();
    printf("All tests passed.\n");
    return 0;
}
