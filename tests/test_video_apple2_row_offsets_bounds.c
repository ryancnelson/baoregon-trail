/*
 * tests/test_video_apple2_row_offsets_bounds.c -- unit test verifying hires_line_offsets
 * table entries stay strictly within 8KB Hi-Res page bounds for all 192 rows.
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../src/video_apple2.h"

static uint8_t mock_read6502(uint16_t addr) {
    (void)addr;
    return 0x00;
}

int main(void) {
    /* Verify row 0 offset is 0 */
    assert(hires_line_offsets[0] == 0x0000);

    /* Verify all 192 row offsets + 39 (40 bytes per line) fit in 8KB (0x2000 bytes) */
    for (int r = 0; r < HIRES_ROWS; r++) {
        uint16_t start_off = hires_line_offsets[r];
        uint16_t end_off = start_off + HIRES_COLS_BYTES - 1;
        assert(end_off < 0x2000u);
    }

    /* Verify out of bounds rows (-1 and 192) are silent no-ops and do not crash or write pixels */
    uint8_t pixels[HIRES_PIXELS_WIDE];
    memset(pixels, 0xAB, sizeof(pixels));

    hires_decode_scanline_mono(-1, mock_read6502, pixels);
    assert(pixels[0] == 0xAB);

    hires_decode_scanline_mono(192, mock_read6502, pixels);
    assert(pixels[0] == 0xAB);

    printf("PASS: video_apple2 row offsets and bounds safety verified\n");
    printf("All tests passed.\n");
    return 0;
}
