/*
 * disk_sector_layout.c -- DOS 3.3 flat .dsk sector-offset math.
 *
 * Minimal implementation to satisfy tests/test_disk_sector_layout.c.
 * See disk_sector_layout.h for the layout rationale.
 */
#include "disk_sector_layout.h"

int dos33_sector_offset(uint8_t track, uint8_t sector, uint32_t *out_offset) {
    if (out_offset == 0) {
        return -1;
    }
    if (track >= DOS33_TRACKS || sector >= DOS33_SECTORS_PER_TRACK) {
        return -1;
    }

    *out_offset = ((uint32_t)track * DOS33_SECTORS_PER_TRACK + sector) * DOS33_SECTOR_SIZE;
    return 0;
}
