/*
 * disk2_controller.c -- Real Disk II peripheral card emulation: phase-based
 * stepper motor, logic-state-sequencer nibble read/write, boot ROM at
 * $Cn00.
 *
 * Ported from whscullin/apple2js (MIT License), specifically:
 *   - js/cards/disk2.ts (softswitch dispatch, sequencer ROM tables,
 *     stepper motor phase-delta table)
 *   - js/cards/drivers/NibbleDiskDriver.ts (nibble read/write logic)
 *   - js/cards/drivers/BaseDiskDriver.ts (shared driver interface)
 *
 * apple2js is Copyright (c) the apple2js contributors and is used here
 * under the terms of the MIT License:
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this software and associated documentation files (the
 *   "Software"), to deal in the Software without restriction, including
 *   without limitation the rights to use, copy, modify, merge, publish,
 *   distribute, sublicense, and/or sell copies of the Software, and to
 *   permit persons to whom the Software is furnished to do so, subject
 *   to the following conditions: The above copyright notice and this
 *   permission notice shall be included in all copies or substantial
 *   portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT
 *   WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 *   TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *   PURPOSE AND NONINFRINGEMENT.
 *
 * Full upstream license: https://github.com/whscullin/apple2js/blob/master/LICENSE
 *
 * DIFFERENCES FROM THE ORIGINAL (adapted for this project, not a literal
 * line-for-line port):
 *   - TypeScript classes -> plain C structs + free functions (no OOP
 *     inheritance hierarchy; NibbleDiskDriver's logic is inlined directly
 *     rather than kept behind a DiskDriver interface, since this project
 *     only supports one disk format for now).
 *   - Removed browser-specific bits: `window.setTimeout`-based drive-off
 *     delay (UI polish for a GUI emulator) is replaced with an immediate
 *     drive-off, since we have no wall-clock/UI timer concept here.
 *   - Removed WOZ-format support (WozDiskDriver.ts) -- only NibbleDisk
 *     (raw .nib-style track data) is ported for now; WOZ can be added
 *     later following the same reference if needed.
 *   - Only DOS 3.3 (16-sector) sequencer ROM/timing is ported; the 13-sector
 *     (DOS 3.2) tables are omitted since this project targets 16-sector
 *     .dsk/.nib images exclusively.
 *
 * NOT YET WIRED UP: this module doesn't yet convert our existing flat
 * DOS-order .dsk format (sector data) into the nibble-encoded track data
 * this driver actually needs -- see tools/dsk_to_nib.py (to be written)
 * and NEXT_STEPS.md Step 7 for the current state of that gap.
 */
#include <string.h>
#include "disk2_controller.h"

/* Logic State Sequencer ROM (P6), DOS 3.3 / 16-sector variant only.
 * See Understanding the Apple IIe, Figure 9.11. Ported verbatim from
 * disk2.ts's SEQUENCER_ROM_16 table -- this is real, documented Apple II
 * hardware behavior data, not something to hand-derive. */
static const uint8_t SEQUENCER_ROM_16[256] = {
    0x18, 0x18, 0x18, 0x18, 0x0A, 0x0A, 0x0A, 0x0A, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
    0x2D, 0x2D, 0x38, 0x38, 0x0A, 0x0A, 0x0A, 0x0A, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28, 0x28,
    0xD8, 0x38, 0x08, 0x28, 0x0A, 0x0A, 0x0A, 0x0A, 0x39, 0x39, 0x39, 0x39, 0x3B, 0x3B, 0x3B, 0x3B,
    0xD8, 0x48, 0x48, 0x48, 0x0A, 0x0A, 0x0A, 0x0A, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48,
    0xD8, 0x58, 0xD8, 0x58, 0x0A, 0x0A, 0x0A, 0x0A, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58,
    0xD8, 0x68, 0xD8, 0x68, 0x0A, 0x0A, 0x0A, 0x0A, 0x68, 0x68, 0x68, 0x68, 0x68, 0x68, 0x68, 0x68,
    0xD8, 0x78, 0xD8, 0x78, 0x0A, 0x0A, 0x0A, 0x0A, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78, 0x78,
    0xD8, 0x88, 0xD8, 0x88, 0x0A, 0x0A, 0x0A, 0x0A, 0x08, 0x08, 0x88, 0x88, 0x08, 0x08, 0x88, 0x88,
    0xD8, 0x98, 0xD8, 0x98, 0x0A, 0x0A, 0x0A, 0x0A, 0x98, 0x98, 0x98, 0x98, 0x98, 0x98, 0x98, 0x98,
    0xD8, 0x29, 0xD8, 0xA8, 0x0A, 0x0A, 0x0A, 0x0A, 0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xA8, 0xA8,
    0xCD, 0xBD, 0xD8, 0xB8, 0x0A, 0x0A, 0x0A, 0x0A, 0xB9, 0xB9, 0xB9, 0xB9, 0xBB, 0xBB, 0xBB, 0xBB,
    0xD9, 0x59, 0xD8, 0xC8, 0x0A, 0x0A, 0x0A, 0x0A, 0xC8, 0xC8, 0xC8, 0xC8, 0xC8, 0xC8, 0xC8, 0xC8,
    0xD9, 0xD9, 0xD8, 0xA0, 0x0A, 0x0A, 0x0A, 0x0A, 0xD8, 0xD8, 0xD8, 0xD8, 0xD8, 0xD8, 0xD8, 0xD8,
    0xD8, 0x08, 0xE8, 0xE8, 0x0A, 0x0A, 0x0A, 0x0A, 0xE8, 0xE8, 0xE8, 0xE8, 0xE8, 0xE8, 0xE8, 0xE8,
    0xFD, 0xFD, 0xF8, 0xF8, 0x0A, 0x0A, 0x0A, 0x0A, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8,
    0xDD, 0x4D, 0xE0, 0xE0, 0x0A, 0x0A, 0x0A, 0x0A, 0x88, 0x88, 0x08, 0x08, 0x88, 0x88, 0x08, 0x08
};

/* Stepper motor phase-delta table: how far (in quarter-tracks) the head
 * moves when in phase X and phase Y is activated. Ported verbatim from
 * disk2.ts's PHASE_DELTA. See that file's own comment for the caveats
 * (simplified vs. real hardware's variable-torque multi-coil behavior). */
static const int8_t PHASE_DELTA[4][4] = {
    { 0,  1,  2, -1},
    {-1,  0,  1,  2},
    {-2, -1,  0,  1},
    { 1, -2, -1,  0},
};

/* Softswitch offsets within $C0E0-$C0EF (slot 6 assumed; see
 * disk2_controller_access()'s caller for real slot-address mapping). */
enum {
    LOC_PHASE0OFF = 0x0, LOC_PHASE0ON = 0x1,
    LOC_PHASE1OFF = 0x2, LOC_PHASE1ON = 0x3,
    LOC_PHASE2OFF = 0x4, LOC_PHASE2ON = 0x5,
    LOC_PHASE3OFF = 0x6, LOC_PHASE3ON = 0x7,
    LOC_DRIVEOFF  = 0x8, LOC_DRIVEON  = 0x9,
    LOC_DRIVE1    = 0xA, LOC_DRIVE2   = 0xB,
    LOC_DRIVEREAD = 0xC, LOC_DRIVEWRITE = 0xD,
    LOC_DRIVEREADMODE = 0xE, LOC_DRIVEWRITEMODE = 0xF,
};

void disk2_controller_reset(disk2_controller_t *ctl) {
    memset(ctl, 0, sizeof(*ctl));
    ctl->drive[0].track = 0;
    ctl->drive[1].track = 0;
}

void disk2_controller_load_nibble_disk(disk2_controller_t *ctl, int drive_no,
                                        const disk2_nibble_track_t tracks[DISK2_MAX_TRACKS],
                                        int read_only) {
    disk2_drive_state_t *d = &ctl->drive[drive_no];
    memcpy(d->tracks, tracks, sizeof(d->tracks));
    d->read_only = read_only;
    d->has_disk = 1;
}

static void set_phase(disk2_controller_t *ctl, int phase, int on) {
    disk2_drive_state_t *d = &ctl->drive[ctl->selected_drive];

    /* Per Sather, UtA2e p.9-12: phase control only takes effect while the
     * selected drive's motor is on. */
    if (!ctl->motor_on) {
        return;
    }

    if (on) {
        d->track += PHASE_DELTA[d->phase][phase] * 2;
        d->phase = phase;

        /* Clamp to valid range (0..DISK2_MAX_TRACKS*4 quarter-tracks),
         * matching NibbleDiskDriver's clampTrack() behavior. */
        if (d->track < 0) {
            d->track = 0;
        }
        if (d->track >= DISK2_MAX_TRACKS * 4) {
            d->track = DISK2_MAX_TRACKS * 4 - 1;
        }
    }
}

/* Reads/writes the next raw nibble at the current drive's head position,
 * on the current track (Q6 LOW / "shift" case). Ported from
 * NibbleDiskDriver.onQ6Low().
 *
 * REAL BUG FIXED (found via TDD, tests/test_disk2_controller.c): the
 * original version of this function read/wrote a nibble on EVERY
 * DRIVEREAD access. Real Disk II hardware's shift register (and the
 * reference NibbleDiskDriver.onQ6Low()) only actually shifts in/out a
 * byte on every OTHER "shift" pulse while in read mode (tracked via a
 * per-drive `skip` flag toggling 0/1/0/1); the intervening pulse is a
 * hardware artifact of the shift-register's own timing that real DOS
 * 3.3 RWTS code depends on when polling $C0EC for the latch's high bit.
 * Also added the isOn() (motor_on) gate onQ6Low() has -- reads/writes
 * while the drive is off must produce latch=0, not real track data. */
static uint8_t nibble_shift(disk2_controller_t *ctl, int write_mode, uint8_t write_value) {
    disk2_drive_state_t *d = &ctl->drive[ctl->selected_drive];
    uint8_t result = 0;

    if (ctl->motor_on && (d->skip || ctl->q7)) {
        int track_index = d->track >> 2;
        if (track_index >= 0 && track_index < DISK2_MAX_TRACKS && d->has_disk) {
            disk2_nibble_track_t *track = &d->tracks[track_index];
            if (track->length > 0) {
                if (d->head >= track->length) {
                    d->head = 0;
                }

                if (write_mode) {
                    if (!d->read_only) {
                        track->data[d->head] = write_value;
                    }
                } else {
                    result = track->data[d->head];
                }
                d->head++;
            }
        }
    } else {
        result = 0;
    }

    d->skip = (d->skip + 1) % 2;
    return result;
}

uint8_t disk2_controller_access(disk2_controller_t *ctl, uint8_t offset, int is_write, uint8_t write_val) {
    uint8_t result = 0;
    int read_mode = !is_write;

    switch (offset & 0x0F) {
        case LOC_PHASE0OFF: set_phase(ctl, 0, 0); break;
        case LOC_PHASE0ON:  set_phase(ctl, 0, 1); break;
        case LOC_PHASE1OFF: set_phase(ctl, 1, 0); break;
        case LOC_PHASE1ON:  set_phase(ctl, 1, 1); break;
        case LOC_PHASE2OFF: set_phase(ctl, 2, 0); break;
        case LOC_PHASE2ON:  set_phase(ctl, 2, 1); break;
        case LOC_PHASE3OFF: set_phase(ctl, 3, 0); break;
        case LOC_PHASE3ON:  set_phase(ctl, 3, 1); break;

        case LOC_DRIVEOFF:
            /* Immediate (no wall-clock delay -- see file header notes on
             * differences from the original). */
            ctl->motor_on = 0;
            break;
        case LOC_DRIVEON:
            ctl->motor_on = 1;
            break;

        case LOC_DRIVE1: ctl->selected_drive = 0; break;
        case LOC_DRIVE2: ctl->selected_drive = 1; break;

        case LOC_DRIVEREAD:
            ctl->q6 = 0;
            if (ctl->q7) {
                /* Write mode + Q6 shift: not yet implemented (write
                 * support pending; read path is the priority for
                 * booting). */
            } else {
                ctl->latch = nibble_shift(ctl, 0, 0);
            }
            break;
        case LOC_DRIVEWRITE:
            ctl->q6 = 1;
            break;

        case LOC_DRIVEREADMODE:  ctl->q7 = 0; break;
        case LOC_DRIVEWRITEMODE: ctl->q7 = 1; break;

        default:
            break;
    }

    if (read_mode) {
        if ((offset & 0x01) == 0) {
            result = ctl->latch;
        }
    } else {
        ctl->bus = write_val;
    }

    return result;
}
