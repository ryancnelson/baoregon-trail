#ifndef WOZ_DISK_H
#define WOZ_DISK_H

#include <stdint.h>
#include <stddef.h>

/*
 * BIO Core 0 / Disk Controller domain: WOZ 1.0 and 2.0 disk image parser.
 *
 * WOZ format is the Apple II preservation standard defined by the Applesauce
 * project. It stores raw bitstream timing and quarter-track data for 5.25"
 * and 3.5" floppy disks.
 *
 * Specification references:
 *   - WOZ 1.0 format specification (Applesauce FD / A2-WOZ1)
 *   - WOZ 2.0 format specification (Applesauce FD / A2-WOZ2)
 */

#define WOZ_MAGIC_WOZ1 0x315A4F57u /* "WOZ1" little-endian uint32 */
#define WOZ_MAGIC_WOZ2 0x325A4F57u /* "WOZ2" little-endian uint32 */

#define WOZ_MAX_TRACKS 160 /* 80 tracks x 2 quarter-tracks */

typedef struct {
    uint8_t bits;          /* pointer to raw bitstream bytes */
    uint32_t bit_count;    /* total bits in track bitstream */
    uint16_t byte_count;   /* total byte length of bitstream */
} woz_track_t;

typedef struct {
    uint8_t version;               /* 1 for WOZ1, 2 for WOZ2 */
    uint8_t disk_type;             /* 1 = 5.25", 2 = 3.5" */
    uint8_t write_protected;       /* 0 or 1 */
    uint8_t synchronized;          /* 0 or 1 */
    uint8_t cleaned_read;          /* 0 or 1 */
    uint8_t tmap[WOZ_MAX_TRACKS];  /* quarter-track entry -> track index (0xFF = unmapped) */

    const uint8_t *track_bits[WOZ_MAX_TRACKS];
    uint32_t track_bit_count[WOZ_MAX_TRACKS];
    uint16_t track_byte_count[WOZ_MAX_TRACKS];
} woz_disk_t;

/*
 * Parse raw WOZ file memory image into out_disk structure.
 * Returns 0 on success, -1 on invalid magic/corrupt data.
 * Safety: NULL input or buffer_size < 12 returns -1 safely.
 */
int woz_parse_image(const uint8_t *buffer, size_t buffer_size, woz_disk_t *out_disk);

/*
 * Read bit value (0 or 1) at bit_index from specified quarter-track (0..159).
 * Returns 0 if quarter-track is unmapped (0xFF) or bit_index exceeds bit_count.
 */
uint8_t woz_read_bit(const woz_disk_t *disk, uint8_t quarter_track, uint32_t bit_index);

#endif /* WOZ_DISK_H */
