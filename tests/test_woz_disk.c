#include <stdio.h>
#include <string.h>

#include "../src/woz_disk.h"

static int test_parse_invalid_buffer_returns_error(void) {
    woz_disk_t disk;
    if (woz_parse_image(NULL, 0, &disk) == 0) {
        fprintf(stderr, "FAIL: woz_parse_image(NULL) should return -1\n");
        return 1;
    }

    uint8_t bad_header[12] = { 'B', 'A', 'D', '!', 0xFF, 0x0D, 0x0A, 0x7F, 0, 0, 0, 0 };
    if (woz_parse_image(bad_header, sizeof(bad_header), &disk) == 0) {
        fprintf(stderr, "FAIL: woz_parse_image with bad magic should return -1\n");
        return 1;
    }

    printf("PASS: test_parse_invalid_buffer_returns_error\n");
    return 0;
}

static int test_parse_minimal_woz1_image(void) {
    uint8_t image[12 + 8 + 60 + 8 + 160 + 8 + (160 * 6656)];
    memset(image, 0, sizeof(image));

    /* Header: "WOZ1" + 0xFF + \r\n\x7F */
    image[0] = 'W'; image[1] = 'O'; image[2] = 'Z'; image[3] = '1';
    image[4] = 0xFF; image[5] = 0x0D; image[6] = 0x0A; image[7] = 0x7F;

    /* Chunk 1: INFO (60 bytes) */
    size_t off = 12;
    image[off + 0] = 'I'; image[off + 1] = 'N'; image[off + 2] = 'F'; image[off + 3] = 'O';
    image[off + 4] = 60; image[off + 5] = 0; image[off + 6] = 0; image[off + 7] = 0;
    off += 8;
    image[off + 0] = 1; /* version = 1 */
    image[off + 1] = 1; /* disk_type = 5.25" */
    off += 60;

    /* Chunk 2: TMAP (160 bytes) */
    image[off + 0] = 'T'; image[off + 1] = 'M'; image[off + 2] = 'A'; image[off + 3] = 'P';
    image[off + 4] = 160; image[off + 5] = 0; image[off + 6] = 0; image[off + 7] = 0;
    off += 8;
    memset(&image[off], 0xFF, 160);
    image[off + 0] = 0; /* quarter-track 0 maps to track entry 0 */
    off += 160;

    /* Chunk 3: TRKS (160 * 6656 bytes) */
    image[off + 0] = 'T'; image[off + 1] = 'R'; image[off + 2] = 'K'; image[off + 3] = 'S';
    uint32_t trks_size = 160 * 6656;
    image[off + 4] = (uint8_t)(trks_size);
    image[off + 5] = (uint8_t)(trks_size >> 8);
    image[off + 6] = (uint8_t)(trks_size >> 16);
    image[off + 7] = (uint8_t)(trks_size >> 24);
    off += 8;

    /* Populate track 0 bitstream: first byte = 0xD5 (11010101) */
    image[off + 0] = 0xD5;
    /* Bytes used = 6400, Bit count = 51200 */
    image[off + 6400] = 0x00; image[off + 6401] = 0x19; /* 6400 */
    image[off + 6402] = 0x00; image[off + 6403] = 0xC8; /* 51200 */

    woz_disk_t disk;
    if (woz_parse_image(image, sizeof(image), &disk) != 0) {
        fprintf(stderr, "FAIL: woz_parse_image failed on valid WOZ1 image\n");
        return 1;
    }

    if (disk.version != 1 || disk.disk_type != 1) {
        fprintf(stderr, "FAIL: version/disk_type mismatch: got version=%d, type=%d\n",
                disk.version, disk.disk_type);
        return 1;
    }

    if (disk.tmap[0] != 0 || disk.tmap[1] != 0xFF) {
        fprintf(stderr, "FAIL: TMAP values incorrect: tmap[0]=%d, tmap[1]=%d\n",
                disk.tmap[0], disk.tmap[1]);
        return 1;
    }

    /* Bit 0..7 of 0xD5 (11010101): MSB-first -> bit0=1, bit1=1, bit2=0, bit3=1, bit4=0, bit5=1, bit6=0, bit7=1 */
    uint8_t b0 = woz_read_bit(&disk, 0, 0);
    uint8_t b1 = woz_read_bit(&disk, 0, 1);
    uint8_t b2 = woz_read_bit(&disk, 0, 2);
    uint8_t b3 = woz_read_bit(&disk, 0, 3);
    uint8_t b4 = woz_read_bit(&disk, 0, 4);

    if (b0 != 1 || b1 != 1 || b2 != 0 || b3 != 1 || b4 != 0) {
        fprintf(stderr, "FAIL: woz_read_bit bits 0..4 = %d %d %d %d %d, expected 1 1 0 1 0\n",
                b0, b1, b2, b3, b4);
        return 1;
    }

    printf("PASS: test_parse_minimal_woz1_image\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_parse_invalid_buffer_returns_error();
    failures += test_parse_minimal_woz1_image();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
