#ifndef DISK_SECTOR_LAYOUT_H
#define DISK_SECTOR_LAYOUT_H

#include <stdint.h>

/*
 * DOS 3.3 / ProDOS logical disk image layout (Duke's domain).
 *
 * Per BRAINSTORM.md section 4 and NEXT_STEPS.md Step 3: we deliberately do
 * NOT emulate the physical Disk II stepper motor or raw GCR nibble tracks.
 * Instead the .dsk image on disk (and in ReRAM) is assumed to already be in
 * "DOS order" -- i.e. a flat sequential byte array of
 * (track * sectors_per_track + sector) * 256-byte sectors, matching the
 * standard 5.25" 140KB Apple II floppy image format used by every DOS 3.3
 * .dsk file (this is the de facto standard produced by all Apple II disk
 * imaging tools; see "Beneath Apple DOS" chapter 3 for the physical format
 * this abstracts over).
 *
 * This header verifies that structural assumption BEFORE tools/embed_disk.py
 * or the $C0E0-$C0EF fast-sector-read trap are written, per CLAUDE.md's
 * TDD requirement ("verify actual .dsk file structure/sector layout
 * carefully before writing the embed tool").
 */

#define DOS33_TRACKS            35
#define DOS33_SECTORS_PER_TRACK 16
#define DOS33_SECTOR_SIZE       256
#define DOS33_DISK_IMAGE_SIZE   (DOS33_TRACKS * DOS33_SECTORS_PER_TRACK * DOS33_SECTOR_SIZE)

/*
 * Compute the byte offset of (track, sector) within a flat DOS-order .dsk
 * image. Returns 0 on success and writes the offset to *out_offset.
 * Returns -1 if track or sector is out of range (track >= DOS33_TRACKS or
 * sector >= DOS33_SECTORS_PER_TRACK), or if out_offset is NULL;
 * *out_offset is left untouched in either error case.
 */
int dos33_sector_offset(uint8_t track, uint8_t sector, uint32_t *out_offset);

#endif /* DISK_SECTOR_LAYOUT_H */
