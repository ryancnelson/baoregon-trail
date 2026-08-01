#include <stdio.h>
#include <string.h>

#include "../src/video_apple2.h"

/*
 * RED test: cross-byte-boundary and scanline-edge adjacency for color
 * artifacting. All existing color tests (test_video_apple2_color.c) only
 * set bits within a single byte -- they never test whether "adjacent lit
 * pixel" detection correctly spans a BYTE boundary (pixel 6 of byte N vs
 * pixel 0 of byte N+1 are adjacent screen columns, cols 6 and 7), nor
 * whether the first/last column of the whole 280-pixel scanline are
 * handled without reading out of bounds or wrongly merging past the edge.
 */

static uint8_t g_mock_row0[HIRES_COLS_BYTES];

static uint8_t mock_read6502(uint16_t address) {
    uint16_t offset = address - HIRES_BASE_ADDR;
    if (offset < HIRES_COLS_BYTES) {
        return g_mock_row0[offset];
    }
    return 0x00;
}

static int expect_color(const char *label, hires_color_t got, hires_color_t want) {
    if (got != want) {
        fprintf(stderr, "FAIL: %s = %d, expected %d\n", label, got, want);
        return 1;
    }
    return 0;
}

static int test_adjacent_lit_pixels_merge_to_white_across_byte_boundary(void) {
    memset(g_mock_row0, 0x00, sizeof(g_mock_row0));
    /* byte 0 bit6 = col 6 (last pixel of byte 0); byte 1 bit0 = col 7
     * (first pixel of byte 1). These are adjacent screen columns even
     * though they come from different bytes. */
    g_mock_row0[0] = 0x40; /* bit6 set -> col 6 lit */
    g_mock_row0[1] = 0x01; /* bit0 set -> col 7 lit */

    hires_color_t out[HIRES_PIXELS_WIDE];
    hires_decode_scanline_color(0, mock_read6502, out);

    int failures = 0;
    failures += expect_color("col6", out[6], HIRES_COLOR_WHITE);
    failures += expect_color("col7", out[7], HIRES_COLOR_WHITE);
    if (!failures) printf("PASS: test_adjacent_lit_pixels_merge_to_white_across_byte_boundary\n");
    return failures;
}

static int test_isolated_pixels_either_side_of_byte_boundary_keep_their_colors(void) {
    memset(g_mock_row0, 0x00, sizeof(g_mock_row0));
    /* col 6 lit alone (byte 0 bit6), col 8 lit alone (byte 1 bit1) -- col 7
     * stays unlit in between, so neither should merge to white. */
    g_mock_row0[0] = 0x40; /* bit6 -> col 6 */
    g_mock_row0[1] = 0x02; /* bit1 -> col 8 */

    hires_color_t out[HIRES_PIXELS_WIDE];
    hires_decode_scanline_color(0, mock_read6502, out);

    /* col 6 is even, palette 0 -> GREEN. col 7 unlit -> BLACK.
     * col 8 is even, palette 0 -> GREEN. */
    int failures = 0;
    failures += expect_color("col6", out[6], HIRES_COLOR_GREEN);
    failures += expect_color("col7", out[7], HIRES_COLOR_BLACK);
    failures += expect_color("col8", out[8], HIRES_COLOR_GREEN);
    if (!failures) printf("PASS: test_isolated_pixels_either_side_of_byte_boundary_keep_their_colors\n");
    return failures;
}

static int test_first_column_of_scanline_does_not_read_out_of_bounds(void) {
    memset(g_mock_row0, 0x00, sizeof(g_mock_row0));
    g_mock_row0[0] = 0x01; /* col 0 lit, isolated (col -1 doesn't exist) */

    hires_color_t out[HIRES_PIXELS_WIDE];
    /* No out-of-bounds read should occur; if it does, this either crashes
     * (caught by a nonzero exit from the test binary) or corrupts the
     * result -- either way this test's assertion catches a regression. */
    hires_decode_scanline_color(0, mock_read6502, out);

    int failures = expect_color("col0", out[0], HIRES_COLOR_GREEN);
    if (!failures) printf("PASS: test_first_column_of_scanline_does_not_read_out_of_bounds\n");
    return failures;
}

static int test_last_column_of_scanline_does_not_read_out_of_bounds(void) {
    memset(g_mock_row0, 0x00, sizeof(g_mock_row0));
    /* Last pixel of the scanline is byte 39 (HIRES_COLS_BYTES-1), bit 6
     * (280 pixels = 40 bytes * 7 bits, so pixel 279 is byte 39's bit 6). */
    g_mock_row0[HIRES_COLS_BYTES - 1] = 0x40;

    hires_color_t out[HIRES_PIXELS_WIDE];
    hires_decode_scanline_color(0, mock_read6502, out);

    /* col 279 is odd (279 % 2 == 1), palette 0 -> VIOLET, isolated (no
     * col 280 exists). */
    int failures = expect_color("col279", out[HIRES_PIXELS_WIDE - 1], HIRES_COLOR_VIOLET);
    if (!failures) printf("PASS: test_last_column_of_scanline_does_not_read_out_of_bounds\n");
    return failures;
}

int main(void) {
    int failures = 0;
    failures += test_adjacent_lit_pixels_merge_to_white_across_byte_boundary();
    failures += test_isolated_pixels_either_side_of_byte_boundary_keep_their_colors();
    failures += test_first_column_of_scanline_does_not_read_out_of_bounds();
    failures += test_last_column_of_scanline_does_not_read_out_of_bounds();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
