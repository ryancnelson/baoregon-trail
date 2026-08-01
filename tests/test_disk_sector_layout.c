/*
 * RED test: verify the DOS 3.3 logical disk image sector-layout math BEFORE
 * writing tools/embed_disk.py or the $C0E0-$C0EF fast-sector-read trap.
 *
 * No real .dsk file is needed for this test -- the DOS-order layout (35
 * tracks x 16 sectors x 256 bytes, sequential) is a fixed, documented
 * format, not something we're guessing at. This test locks in that
 * structural understanding as an executable spec so a later wrong
 * assumption in the embed tool or trap gets caught immediately.
 */
#include <stdio.h>
#include "../src/disk_sector_layout.h"

static int failures = 0;

static void check_offset(uint8_t track, uint8_t sector, uint32_t expected) {
    uint32_t offset = 0xFFFFFFFFu;
    int rc = dos33_sector_offset(track, sector, &offset);
    if (rc != 0) {
        fprintf(stderr, "FAIL: dos33_sector_offset(track=%u, sector=%u) returned error, expected success\n",
                track, sector);
        failures++;
        return;
    }
    if (offset != expected) {
        fprintf(stderr, "FAIL: dos33_sector_offset(track=%u, sector=%u) = %u, expected %u\n",
                track, sector, offset, expected);
        failures++;
        return;
    }
    printf("PASS: dos33_sector_offset(track=%u, sector=%u) == %u\n", track, sector, expected);
}

static void check_rejects_out_of_range(uint8_t track, uint8_t sector, const char *label) {
    uint32_t offset = 0;
    int rc = dos33_sector_offset(track, sector, &offset);
    if (rc == 0) {
        fprintf(stderr, "FAIL: dos33_sector_offset(%s) should have been rejected, got offset=%u\n",
                label, offset);
        failures++;
        return;
    }
    printf("PASS: dos33_sector_offset(%s) correctly rejected\n", label);
}

int main(void) {
    /* Total image size must match the standard 140KB Apple II 5.25" image. */
    if (DOS33_DISK_IMAGE_SIZE != 143360) {
        fprintf(stderr, "FAIL: DOS33_DISK_IMAGE_SIZE = %d, expected 143360\n", DOS33_DISK_IMAGE_SIZE);
        failures++;
    } else {
        printf("PASS: DOS33_DISK_IMAGE_SIZE == 143360 (140KB)\n");
    }

    /* First sector of the disk: track 0, sector 0 -> offset 0. */
    check_offset(0, 0, 0);

    /* DOS 3.3's VTOC/catalog lives at track 17 -- known reference point,
     * useful smoke test that track skew lands where every DOS 3.3 reference
     * says it should: (17*16 + 0) * 256 = 69632. */
    check_offset(17, 0, 69632);

    /* Last sector of the last track: track 34, sector 15 -- must be the
     * final 256-byte block of the image (offset + 256 == total size). */
    check_offset(34, 15, 143360 - 256);

    /* Out-of-range track/sector must be rejected, not silently wrap or
     * read garbage -- a bad request must never scribble into the wrong
     * ReRAM offset. */
    check_rejects_out_of_range(35, 0, "track=35 (out of range)");
    check_rejects_out_of_range(0, 16, "sector=16 (out of range)");

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
