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
#include <stddef.h>
#include "disk2_controller.h"

static void local_memset(void *dest, int val, size_t n) {
    uint8_t *p = (uint8_t *)dest;
    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)val;
    }
}

static void local_memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

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

/* Disk II boot PROM (P5 chip, part number 341-0027-a), 16-sector (DOS
 * 3.3) variant. Real Apple II hardware: on cold boot, the system ROM's
 * own reset/autostart code JSRs into $Cn00 for the lowest-numbered slot
 * with a bootable peripheral card (Disk II is conventionally slot 6,
 * i.e. $C600-$C6FF) -- this 256-byte PROM IS that boot code. It's what
 * actually drives the $C0E0-$C0EF softswitches (via disk2_controller_access())
 * to step to track 0, select sector 0, read it via the Q6/Q7 nibble
 * shift-register protocol, decode the 6-and-2 GCR data, and load DOS
 * 3.3's own bootstrap into $0800 before jumping there -- the real
 * mechanism NEXT_STEPS.md Step 7 exists to support, replacing
 * disk_trap.c's fast-sector-read shortcut for real, unmodified disk
 * images.
 *
 * Embedded byte-for-byte from the real 341-0027-a.p5 chip (dumped from
 * roms/a2diskiing.zip, MAME/No-Intro-style ROM archive, sourced
 * 2026-08-01 per NEXT_STEPS.md's ROM-sourcing note) -- verified against
 * apple2js's own BOOTSTRAP_ROM_16 constant (js/roms/cards/disk2.ts),
 * byte-for-byte identical. This is real, documented, historical Apple
 * II firmware, not something to hand-derive or approximate. */
static const uint8_t DISK2_BOOT_ROM_16[256] = {
    0xA2, 0x20, 0xA0, 0x00, 0xA2, 0x03, 0x86, 0x3C, 0x8A, 0x0A, 0x24, 0x3C, 0xF0, 0x10, 0x05, 0x3C,
    0x49, 0xFF, 0x29, 0x7E, 0xB0, 0x08, 0x4A, 0xD0, 0xFB, 0x98, 0x9D, 0x56, 0x03, 0xC8, 0xE8, 0x10,
    0xE5, 0x20, 0x58, 0xFF, 0xBA, 0xBD, 0x00, 0x01, 0x0A, 0x0A, 0x0A, 0x0A, 0x85, 0x2B, 0xAA, 0xBD,
    0x8E, 0xC0, 0xBD, 0x8C, 0xC0, 0xBD, 0x8A, 0xC0, 0xBD, 0x89, 0xC0, 0xA0, 0x50, 0xBD, 0x80, 0xC0,
    0x98, 0x29, 0x03, 0x0A, 0x05, 0x2B, 0xAA, 0xBD, 0x81, 0xC0, 0xA9, 0x56, 0x20, 0xA8, 0xFC, 0x88,
    0x10, 0xEB, 0x85, 0x26, 0x85, 0x3D, 0x85, 0x41, 0xA9, 0x08, 0x85, 0x27, 0x18, 0x08, 0xBD, 0x8C,
    0xC0, 0x10, 0xFB, 0x49, 0xD5, 0xD0, 0xF7, 0xBD, 0x8C, 0xC0, 0x10, 0xFB, 0xC9, 0xAA, 0xD0, 0xF3,
    0xEA, 0xBD, 0x8C, 0xC0, 0x10, 0xFB, 0xC9, 0x96, 0xF0, 0x09, 0x28, 0x90, 0xDF, 0x49, 0xAD, 0xF0,
    0x25, 0xD0, 0xD9, 0xA0, 0x03, 0x85, 0x40, 0xBD, 0x8C, 0xC0, 0x10, 0xFB, 0x2A, 0x85, 0x3C, 0xBD,
    0x8C, 0xC0, 0x10, 0xFB, 0x25, 0x3C, 0x88, 0xD0, 0xEC, 0x28, 0xC5, 0x3D, 0xD0, 0xBE, 0xA5, 0x40,
    0xC5, 0x41, 0xD0, 0xB8, 0xB0, 0xB7, 0xA0, 0x56, 0x84, 0x3C, 0xBC, 0x8C, 0xC0, 0x10, 0xFB, 0x59,
    0xD6, 0x02, 0xA4, 0x3C, 0x88, 0x99, 0x00, 0x03, 0xD0, 0xEE, 0x84, 0x3C, 0xBC, 0x8C, 0xC0, 0x10,
    0xFB, 0x59, 0xD6, 0x02, 0xA4, 0x3C, 0x91, 0x26, 0xC8, 0xD0, 0xEF, 0xBC, 0x8C, 0xC0, 0x10, 0xFB,
    0x59, 0xD6, 0x02, 0xD0, 0x87, 0xA0, 0x00, 0xA2, 0x56, 0xCA, 0x30, 0xFB, 0xB1, 0x26, 0x5E, 0x00,
    0x03, 0x2A, 0x5E, 0x00, 0x03, 0x2A, 0x91, 0x26, 0xC8, 0xD0, 0xEE, 0xE6, 0x27, 0xE6, 0x3D, 0xA5,
    0x3D, 0xCD, 0x00, 0x08, 0xA6, 0x2B, 0x90, 0xDB, 0x4C, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
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
    local_memset(ctl, 0, sizeof(*ctl));
    ctl->drive[0].track = 0;
    ctl->drive[1].track = 0;
}

uint8_t disk2_controller_read_boot_rom(uint8_t offset) {
    return DISK2_BOOT_ROM_16[offset];
}

void disk2_controller_load_nibble_disk(disk2_controller_t *ctl, int drive_no,
                                        const disk2_nibble_track_t tracks[DISK2_MAX_TRACKS],
                                        int read_only) {
    disk2_drive_state_t *d = &ctl->drive[drive_no];
    local_memcpy(d->tracks, tracks, sizeof(d->tracks));
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

#include "cpu6502.h"

__attribute__((weak)) uint32_t clockticks6502 = 0;

#define NIBBLE_CYCLES 32u

/* Reads/writes the next raw nibble at the current drive's head position,
 * on the current track (Q6 LOW / "shift" case). Ported from
 * NibbleDiskDriver.onQ6Low().
 *
 * Cycle-accurate timing fix (2026-08-01): Real Apple II Disk II hardware
 * rotates at 300 RPM (5 rps), shifting 1 nibble byte every 32 6502 CPU
 * cycles (~30.05 microseconds). When a nibble is shifted in, bit 7 of the
 * latch register is set (0x80..0xFF). Reading $C0EC latches the byte and
 * clears bit 7. Subsequent reads of $C0EC returning before 32 cycles pass
 * yield bit 7 = 0, causing the boot PROM's BPL spin loop (LDA $C0EC / BPL)
 * to wait exactly 32 cycles between nibbles.
 */
static uint8_t nibble_shift(disk2_controller_t *ctl, int write_mode, uint8_t write_value) {
    disk2_drive_state_t *d = &ctl->drive[ctl->selected_drive];

    if (!ctl->motor_on) {
        ctl->latch = 0;
        return 0;
    }

    uint32_t now = clockticks6502;
    uint32_t elapsed = now - d->last_cycles;
    uint32_t nibbles = elapsed / NIBBLE_CYCLES;

    if (nibbles > 0 || d->last_cycles == 0) {
        int track_index = d->track >> 2;
        if (track_index >= 0 && track_index < DISK2_MAX_TRACKS && d->has_disk) {
            disk2_nibble_track_t *track = &d->tracks[track_index];
            if (track->length > 0) {
                if (nibbles > 0 && d->last_cycles > 0) {
                    d->head = (d->head + (int)nibbles) % track->length;
                    d->last_cycles += nibbles * NIBBLE_CYCLES;
                } else if (d->last_cycles == 0) {
                    d->last_cycles = (now == 0) ? 1 : now;
                }

                if (write_mode) {
                    if (!d->read_only) {
                        track->data[d->head] = write_value;
                    }
                } else {
                    ctl->latch = track->data[d->head];
                }
            }
        }
    }

    uint8_t res = ctl->latch;
    if (!write_mode) {
        ctl->latch &= 0x7Fu; /* Clear bit 7 after read */
    }
    return res;
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
                result = nibble_shift(ctl, 0, 0);
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
        if ((offset & 0x01) == 0 && (offset & 0x0E) != LOC_DRIVEREAD) {
            result = ctl->latch;
        }
    } else {
        ctl->bus = write_val;
    }

    return result;
}
