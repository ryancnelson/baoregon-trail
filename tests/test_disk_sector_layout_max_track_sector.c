/*
 * tests/test_disk_sector_layout_max_track_sector.c -- unit test verifying
 * dos33_sector_offset() boundary values at maximum track (34) and sector (15).
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "../src/disk_sector_layout.h"

int main(void) {
    assert(DOS33_DISK_IMAGE_SIZE == 143360u);

    uint32_t offset = 0xFFFFFFFFu;

    /* Minimum track 0, sector 0 -> offset 0 */
    assert(dos33_sector_offset(0, 0, &offset) == 0);
    assert(offset == 0u);

    /* Maximum track 34, sector 15 -> offset 143104 ((34 * 16 + 15) * 256) */
    assert(dos33_sector_offset(34, 15, &offset) == 0);
    assert(offset == 143104u);

    /* Out of bounds track 35 -> error, offset untouched */
    offset = 12345u;
    assert(dos33_sector_offset(35, 0, &offset) == -1);
    assert(offset == 12345u);

    /* Out of bounds sector 16 -> error, offset untouched */
    assert(dos33_sector_offset(0, 16, &offset) == -1);
    assert(offset == 12345u);

    printf("PASS: disk_sector_layout max track/sector boundary verified\n");
    printf("All tests passed.\n");
    return 0;
}
