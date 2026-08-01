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
} disk2_drive_state_t;

typedef struct {
    disk2_drive_state_t drive[2]; /* drive 1, drive 2 */
    int selected_drive;           /* 0 or 1 (maps to drive 1/drive 2) */
    int motor_on;
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
