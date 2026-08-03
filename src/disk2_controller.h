/*
 * disk2_controller.h -- see disk2_controller.c for full attribution
 * (ported from whscullin/apple2js, MIT License).
 */
#ifndef DISK2_CONTROLLER_H
#define DISK2_CONTROLLER_H

#include <stdint.h>

/* 35 tracks, standard Apple II Disk II geometry. Quarter-track stepping
 * gives 140 addressable quarter-track positions (0..139). */
#define DISK2_MAX_TRACKS 35

/* Max raw nibble bytes per track. Real Disk II tracks hold roughly
 * 6656 bytes of GCR-encoded nibble data (accounting for sync bytes,
 * address/data fields, gaps); apple2js uses similar sizing for its
 * NibbleDisk format. Sized generously here. */
#define DISK2_MAX_TRACK_BYTES 6656

/* Sentinel cycle-timestamp value meaning "uninitialized" / "not
 * pending" -- shared by disk2_drive_state_t.last_cycles and
 * disk2_controller_t.motor_off_pending_since. */
#define DISK2_CYCLES_UNINIT 0xFFFFFFFFu

typedef struct {
    uint8_t data[DISK2_MAX_TRACK_BYTES];
    int length; /* actual bytes used, <= DISK2_MAX_TRACK_BYTES */
} disk2_nibble_track_t;

typedef struct {
    disk2_nibble_track_t tracks[DISK2_MAX_TRACKS];
    int track;       /* current head position, in quarter-tracks (0..139) */
    int phase;       /* which stepper motor phase (0-3) is currently engaged */
    int head;        /* byte offset within the current track's nibble data */
    int read_only;
    int has_disk;
    /* Real Disk II hardware's shift register takes TWO Q6-LOW ("shift")
     * pulses to produce one available nibble -- toggles 0/1/0/1 on every
     * DRIVEREAD access; a byte is only actually latched/head-advanced on
     * alternating pulses (or unconditionally in write mode, per
     * NibbleDiskDriver.onQ6Low()'s `skip || controller.q7` guard). Real
     * DOS 3.3 RWTS polls $C0EC in a tight loop expecting this exact
     * cadence; omitting it desyncs real boot code. */
    int skip;
    uint32_t last_cycles; /* 6502 CPU cycle timestamp of last nibble shift (32 cycles/nibble) */
} disk2_drive_state_t;

typedef struct {
    disk2_drive_state_t drive[2]; /* drive 1, drive 2 */
    int selected_drive;           /* 0 or 1 (maps to drive 1/drive 2) */
    int motor_on;
    /* Real Apple II Disk II hardware doesn't stop the drive motor
     * instantly on a LOC_DRIVEOFF ($C0E8) access -- it starts a real
     * ~1-second spindown timer; the motor keeps physically spinning
     * (and the data latch keeps producing real shifted-in nibbles)
     * until that timer actually fires. A subsequent LOC_DRIVEON
     * ($C0E9) access before the timer fires cancels the pending
     * off entirely. Real 4am-crack boot code (Lode Runner, see
     * NEXT_STEPS.md) depends on this: it accesses $C0E8 then polls
     * $C0EC one more time expecting a real fresh nibble, not a hard
     * zero. `motor_off_pending_since` records the clockticks6502
     * timestamp of the LOC_DRIVEOFF access that started this grace
     * period; DISK2_CYCLES_UNINIT means no spindown is pending (either
     * never started, or cancelled by a LOC_DRIVEON). See
     * tests/test_disk2_controller_motor_spindown_grace_period.c. */
    uint32_t motor_off_pending_since;
    int q6;
    int q7;
    uint8_t latch;
    uint8_t bus;
} disk2_controller_t;

/* Resets the controller and both drives to power-on state (motor off,
 * track 0, no phase engaged). Does NOT clear loaded disk images -- call
 * disk2_controller_load_nibble_disk() again after reset if a fresh load
 * is needed. */
void disk2_controller_reset(disk2_controller_t *ctl);

/* Reads one byte of the embedded 256-byte Disk II boot PROM
 * (341-0027-a.p5, 16-sector/DOS 3.3 variant) at the given offset
 * (0x00-0xFF -- caller is responsible for real slot-address decoding,
 * e.g. slot 6 is $C600-$C6FF, offset = address - 0xC600, same
 * convention as disk2_controller_access()'s $C0E0-$C0EF offset). This
 * is the code real Apple II hardware JSRs into at $Cn00 during cold
 * boot to drive the Disk II softswitches and load a disk's first
 * sector. */
uint8_t disk2_controller_read_boot_rom(uint8_t offset);

/* Loads nibble-encoded track data into the given drive (0 or 1). The
 * caller is responsible for producing real nibble-encoded data (see
 * NEXT_STEPS.md Step 7 -- a .dsk-to-nibble converter is not yet written;
 * this function just installs whatever track data it's given). */
void disk2_controller_load_nibble_disk(disk2_controller_t *ctl, int drive_no,
                                        const disk2_nibble_track_t tracks[DISK2_MAX_TRACKS],
                                        int read_only);

/* Dispatches a read or write access to one of the $C0E0-$C0EF softswitch
 * offsets (offset should already be masked to 0x00-0x0F by the caller,
 * which is responsible for real slot-address decoding -- e.g. slot 6 is
 * $C0E0-$C0EF, offset = address - 0xC0E0). Returns the value read (valid
 * only when is_write is false); write_val is used only when is_write is
 * true. */
uint8_t disk2_controller_access(disk2_controller_t *ctl, uint8_t offset, int is_write, uint8_t write_val);

#endif
