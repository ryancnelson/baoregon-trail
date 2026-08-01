#ifndef APPLE2_MEM_H
#define APPLE2_MEM_H

#include <stdint.h>
#include "bunnie_audio.h"

/*
 * apple2_mem.c -- 64KB Apple II memory map + $C000-$C0FF soft-switch
 * dispatch. Implements the read6502()/write6502() bus functions declared
 * in cpu6502.h (contract locked 2026-07-31: baochip/Woz/Bunnie/Duke).
 *
 * Address map:
 *   $0000-$CFFF, $E000-$FFFF : plain RAM (includes the $2000-$3FFF Hi-Res
 *                               buffer Bunnie's video_apple2.c reads via
 *                               its own read6502_fn parameter -- no special
 *                               casing needed here, it's just RAM).
 *   $C000-$C0FF              : soft-switch dispatch (see below).
 *   $D000-$FFFF              : Apple IIe Autostart ROM region (per
 *                               PROJECT_GOALS.md Milestone 3 / BRAINSTORM.md)
 *                               -- write-protected. NOTE: $D000-$FFFF
 *                               overlaps $E000-$FFFF above; ROM protection
 *                               wins for that overlap (writes there are
 *                               ignored, reads return whatever was loaded
 *                               at init/reset time). Real ROM image loading
 *                               is a later iteration (Step 5); for now the
 *                               region simply enforces write-protection
 *                               semantics over backing RAM bytes.
 *
 * Soft-switch dispatch inside $C000-$C0FF (per baochip's sketch, locked
 * 2026-07-31):
 *   $C030          : Bunnie's speaker toggle -- calls
 *                     bunnie_audio_trigger_toggle() (fire-and-forget,
 *                     non-blocking, mechanism (b) memory-mapped flag).
 *   $C0E0          : Duke's disk trap -- stage a track number (write-only).
 *   $C0E1          : Duke's disk trap -- stage a sector number and select
 *                     the (track, sector) via disk_trap_select_sector();
 *                     resets the internal byte cursor to 0 (write-only).
 *   $C0EC          : Duke's disk trap -- data port. Each read returns the
 *                     next byte of the currently selected 256-byte sector
 *                     via disk_trap_read_byte(), auto-incrementing (and
 *                     wrapping mod 256) so DOS 3.3/ProDOS can stream a
 *                     whole sector with repeated reads of one address --
 *                     no physical GCR/stepper timing, per BRAINSTORM.md
 *                     section 4's explicit simplification.
 *   anything else in $C000-$C0FF : reads as 0x00, writes ignored (soft
 *                     switches not yet implemented are inert, not crashes).
 */

/* Reset the 64KB RAM to all zero, reset the disk trap's byte cursor, and
 * re-initialize Bunnie's audio state. Call once before use and between
 * tests for a clean, deterministic starting point. */
void apple2_mem_reset(void);

/* Register the ReRAM-resident disk image backing the $C0E0-$C0EF disk
 * trap. Thin pass-through to disk_trap_set_image() -- see disk_trap.h. */
void apple2_mem_set_disk_image(const uint8_t *image);

/* Expose Bunnie's audio state so BIO Core 1's polling loop (and tests) can
 * call bunnie_audio_poll_and_apply() on it. apple2_mem.c owns the single
 * instance; this returns a pointer to it, not a copy. */
bunnie_audio_state_t *apple2_mem_get_audio_state(void);

#endif /* APPLE2_MEM_H */
