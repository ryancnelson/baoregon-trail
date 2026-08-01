#include <stdio.h>
#include <string.h>

#include "../src/video_apple2.h"

/*
 * RED test (vertical tracer bullet): prove NTSC artifact color decoding for
 * isolated lit pixels (4-color cases keyed on column parity x palette bit),
 * adjacent lit pixels merging to white, and unlit pixels staying black.
 *
 * Byte format: bits 0-6 are the 7 horizontal pixels (LSB first), bit 7 is
 * the palette-select bit. Reused across cases via a single mock buffer for
 * row 0 (offset 0x0000, per hires_line_offsets[0]).
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

static int test_isolated_pixel_even_col_palette0_is_green(void) {
    memset(g_mock_row0, 0x00, sizeof(g_mock_row0));
    g_mock_row0[0] = 0x01; /* bit0 set (col 0, even), high bit 0 */

    hires_color_t out[HIRES_PIXELS_WIDE];
    hires_decode_scanline_color(0, mock_read6502, out);

    int failures = 0;
    failures += expect_color("col0", out[0], HIRES_COLOR_GREEN);
    failures += expect_color("col1", out[1], HIRES_COLOR_BLACK);
    if (!failures) printf("PASS: test_isolated_pixel_even_col_palette0_is_green\n");
    return failures;
}

static int test_isolated_pixel_odd_col_palette0_is_violet(void) {
    memset(g_mock_row0, 0x00, sizeof(g_mock_row0));
    g_mock_row0[0] = 0x02; /* bit1 set (col 1, odd), high bit 0 */

    hires_color_t out[HIRES_PIXELS_WIDE];
    hires_decode_scanline_color(0, mock_read6502, out);

    int failures = 0;
    failures += expect_color("col0", out[0], HIRES_COLOR_BLACK);
    failures += expect_color("col1", out[1], HIRES_COLOR_VIOLET);
    if (!failures) printf("PASS: test_isolated_pixel_odd_col_palette0_is_violet\n");
    return failures;
}

static int test_isolated_pixel_even_col_palette1_is_orange(void) {
    memset(g_mock_row0, 0x00, sizeof(g_mock_row0));
    g_mock_row0[0] = 0x81; /* high bit set + bit0 set (col 0, even) */

    hires_color_t out[HIRES_PIXELS_WIDE];
    hires_decode_scanline_color(0, mock_read6502, out);

    int failures = expect_color("col0", out[0], HIRES_COLOR_ORANGE);
    if (!failures) printf("PASS: test_isolated_pixel_even_col_palette1_is_orange\n");
    return failures;
}

static int test_isolated_pixel_odd_col_palette1_is_blue(void) {
    memset(g_mock_row0, 0x00, sizeof(g_mock_row0));
    g_mock_row0[0] = 0x82; /* high bit set + bit1 set (col 1, odd) */

    hires_color_t out[HIRES_PIXELS_WIDE];
    hires_decode_scanline_color(0, mock_read6502, out);

    int failures = expect_color("col1", out[1], HIRES_COLOR_BLUE);
    if (!failures) printf("PASS: test_isolated_pixel_odd_col_palette1_is_blue\n");
    return failures;
}

static int test_adjacent_lit_pixels_merge_to_white(void) {
    memset(g_mock_row0, 0x00, sizeof(g_mock_row0));
    g_mock_row0[0] = 0x03; /* bits 0,1 both set -> consecutive lit pixels */

    hires_color_t out[HIRES_PIXELS_WIDE];
    hires_decode_scanline_color(0, mock_read6502, out);

    int failures = 0;
    failures += expect_color("col0", out[0], HIRES_COLOR_WHITE);
    failures += expect_color("col1", out[1], HIRES_COLOR_WHITE);
    if (!failures) printf("PASS: test_adjacent_lit_pixels_merge_to_white\n");
    return failures;
}

static int test_all_unlit_pixels_are_black(void) {
    memset(g_mock_row0, 0x00, sizeof(g_mock_row0));

    hires_color_t out[HIRES_PIXELS_WIDE];
    hires_decode_scanline_color(0, mock_read6502, out);

    for (int i = 0; i < HIRES_PIXELS_WIDE; i++) {
        if (out[i] != HIRES_COLOR_BLACK) {
            fprintf(stderr, "FAIL: pixel[%d] = %d, expected BLACK\n", i, out[i]);
            return 1;
        }
    }
    printf("PASS: test_all_unlit_pixels_are_black\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_isolated_pixel_even_col_palette0_is_green();
    failures += test_isolated_pixel_odd_col_palette0_is_violet();
    failures += test_isolated_pixel_even_col_palette1_is_orange();
    failures += test_isolated_pixel_odd_col_palette1_is_blue();
    failures += test_adjacent_lit_pixels_merge_to_white();
    failures += test_all_unlit_pixels_are_black();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
