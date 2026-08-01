#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../src/video_apple2.h"

/*
 * RED test #1 (vertical tracer bullet): prove the row-0 byte-to-pixel
 * expansion is correct for a single hand-verified byte, using a mock
 * read6502 in place of Woz's real cpu6502.c (not landed yet -- interface
 * contract locked 2026-07-31: uint8_t read6502(uint16_t address)).
 *
 * Known-good reference: Apple II Hi-Res byte format packs 7 pixels into
 * bits 0-6 (LSB first), bit 7 is the palette-shift bit (ignored for this
 * mono-only first pass -- color artifacting is a later iteration per
 * BRAINSTORM.md section 2 step 4).
 *
 * Byte 0x55 = 0b0101_0101 -> bits 0..6 = 1,0,1,0,1,0,1
 * so the first 7 output pixels of row 0 must be {1,0,1,0,1,0,1}.
 * The remaining 39 bytes of the row are 0x00 -> all zero pixels.
 */

static uint8_t g_mock_hires_row0[HIRES_COLS_BYTES];

static uint8_t mock_read6502(uint16_t address) {
    /* Row 0 lives at HIRES_BASE_ADDR ($2000) per hires_line_offsets[0] == 0. */
    uint16_t offset = address - HIRES_BASE_ADDR;
    if (offset < HIRES_COLS_BYTES) {
        return g_mock_hires_row0[offset];
    }
    return 0x00;
}

static int test_decode_scanline_mono_row0_single_byte(void) {
    memset(g_mock_hires_row0, 0x00, sizeof(g_mock_hires_row0));
    g_mock_hires_row0[0] = 0x55; /* 0b0101_0101 */

    uint8_t out_pixels[HIRES_PIXELS_WIDE];
    memset(out_pixels, 0xFF, sizeof(out_pixels)); /* poison to catch no-writes */

    hires_decode_scanline_mono(0, mock_read6502, out_pixels);

    const uint8_t expected_first7[7] = {1, 0, 1, 0, 1, 0, 1};
    for (int i = 0; i < 7; i++) {
        if (out_pixels[i] != expected_first7[i]) {
            fprintf(stderr,
                    "FAIL: pixel[%d] = %u, expected %u\n",
                    i, out_pixels[i], expected_first7[i]);
            return 1;
        }
    }
    for (int i = 7; i < HIRES_PIXELS_WIDE; i++) {
        if (out_pixels[i] != 0) {
            fprintf(stderr, "FAIL: pixel[%d] = %u, expected 0\n", i, out_pixels[i]);
            return 1;
        }
    }
    printf("PASS: test_decode_scanline_mono_row0_single_byte\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_decode_scanline_mono_row0_single_byte();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
