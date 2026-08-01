#ifndef DISK_TRAP_H
#define DISK_TRAP_H

#include <stdint.h>
#include "disk_sector_layout.h"

/*
 * $C0E0-$C0EF Disk II fast-sector-read trap (Duke's domain).
 *
 * Bus interface contract locked 2026-07-31 (baochip/Woz/Bunnie/Duke):
 *   uint8_t read6502(uint16_t address);
 *   void    write6502(uint16_t address, uint8_t value);
 *
 * Per BRAINSTORM.md section 4, we do NOT emulate the physical Disk II
 * stepper motor / raw GCR nibble tracks. Software (DOS 3.3/ProDOS) requests
 * a (track, sector) via two soft-switch writes, then a subsequent access
 * reads 256 bytes back. This trap copies the sector directly out of the
 * ReRAM-resident disk image -- no cross-core signaling needed (unlike
 * Bunnie's $C030 speaker trap): it's a synchronous bulk data substitution
 * inline in read6502/write6502, executing in <10 RISC-V clock cycles per
 * BRAINSTORM.md's target.
 *
 * Trap-notification mechanism decision (2026-07-31, Bunnie/baochip/Duke):
 * settled on (b) memory-mapped flag/register for cross-core signaling
 * (Bunnie's $C030 case). Duke's disk trap needs no such signal -- it's a
 * same-core, same-call data copy, so it does not participate in that
 * mechanism at all.
 *
 * apple2_mem.c is not written yet (blocked on Woz's cpu6502.c stubs
 * landing, per NEXT_STEPS.md Step 3). Per Ryan's steer 2026-07-31: don't
 * block on that -- develop and test this trap logic now against a mock
 * bus matching the locked signatures, swap in the real read6502/write6502
 * wiring once cpu6502.c lands.
 */

/* Register the ReRAM-resident disk image this trap should serve sectors
 * from. image must point to a flat DOS-order buffer of at least
 * DOS33_DISK_IMAGE_SIZE bytes and must outlive any calls to
 * disk_trap_select_sector()/disk_trap_read_byte(). */
void disk_trap_set_image(const uint8_t *image);

/* Returns pointer to currently registered disk image, or NULL if unmounted. */
const uint8_t *disk_trap_get_image_ptr(void);

/* Unload the registered disk image pointer (sets to NULL) and reset selection state. */
void disk_trap_clear_image(void);

/* Reset the trap's (track, sector) SELECTION state only -- as if
 * disk_trap_select_sector() had never been called (subsequent
 * disk_trap_read_byte() calls safely return 0x00 until a new selection
 * is made, matching tests/test_disk_trap_safe_defaults.c's documented
 * contract). Does NOT clear the registered disk image
 * (disk_trap_set_image()) -- a real Disk II's inserted disk stays
 * physically in the drive across a CPU reset; only the drive's
 * currently-latched track/sector registers reset, matching this
 * module's existing $C0E0/$C0E1-select vs $C0EC-stream split. Call this
 * from apple2_mem_reset() so a stale sector selection from before a
 * reset (e.g. the 3-button soft-reset combo, or a failed boot retry)
 * can never leak into a fresh boot attempt -- see
 * tests/test_apple2_mem_reset_clears_disk_trap_selection.c for the
 * end-to-end regression proof (exercises the real apple2_mem_reset() ->
 * disk_trap_reset() wiring through the actual $C0E0/$C0E1/$C0EC
 * softswitch path, not just this module in isolation). */
void disk_trap_reset(void);

/* Returns 1 if a valid (track, sector) selection is currently active,
 * 0 otherwise (post-init, or post-reset before a new sector selection). */
int disk_trap_has_selection(void);

/* Returns the byte offset into the mounted disk image for the currently
 * selected sector (or 0 if no selection has been made yet / post-reset). */
uint32_t disk_trap_get_selected_offset(void);

/* Select the (track, sector) that subsequent disk_trap_read_byte() calls
 * will serve from, mirroring how DOS 3.3 writes track/sector registers to
 * the $C0E0-$C0EF soft-switch range before reading sector data back.
 * Returns 0 on success, -1 if track/sector is out of range (matches
 * dos33_sector_offset()'s contract) -- on error, the previously selected
 * sector (if any) is left unchanged. */
int disk_trap_select_sector(uint8_t track, uint8_t sector);

/* Read one byte at the given offset (0-255) within the currently selected
 * sector. byte_offset's uint8_t type makes it always < DOS33_SECTOR_SIZE
 * (256), so that range is never actually out of bounds. If
 * disk_trap_select_sector() has not yet been called successfully (or its
 * last call failed/was rejected), this safely returns 0x00 rather than
 * reading uninitialized/stale offset state -- see
 * tests/test_disk_trap_safe_defaults.c for the regression proof of this
 * documented-and-tested safe-default behavior. */
uint8_t disk_trap_read_byte(uint8_t byte_offset);

#endif /* DISK_TRAP_H */
