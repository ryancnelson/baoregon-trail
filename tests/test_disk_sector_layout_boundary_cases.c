/*
 * tests/test_disk_sector_layout_boundary_cases.c -- unit test verifying
 * boundary conditions for dos33_sector_offset() at track 34 sector 15
 * (max valid) and track 35 / sector 16 (first invalid).
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "../src/disk_sector_layout.h"

int main(void) {
    uint32_t offset = 0;
    int res;

    /* Track 34, Sector 15: Last valid sector in 143,360 byte DOS 3.3 image */
    res = dos33_sector_offset(34, 15, &offset);
    assert(res == 0);
    /* (34 * 16 + 15) * 256 = (544 + 15) * 256 = 559 * 256 = 143,104 */
    assert(offset == 143104);
    assert(offset + 256 == DOS33_DISK_IMAGE_SIZE);

    /* Track 35, Sector 0: Invalid (track out of bounds) */
    res = dos33_sector_offset(35, 0, &offset);
    assert(res == -1);

    /* Track 0, Sector 16: Invalid (sector out of bounds) */
    res = dos33_sector_offset(0, 16, &offset);
    assert(res == -1);

    /* Track 255, Sector 255: Invalid (both out of bounds) */
    res = dos33_sector_offset(255, 255, &offset);
    assert(res == -1);

    printf("PASS: dos33_sector_offset boundary cases verified\n");
    printf("All tests passed.\n");
    return 0;
}
