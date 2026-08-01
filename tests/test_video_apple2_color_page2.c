#include <stdio.h>
#include <string.h>

#include "../src/video_apple2.h"
#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"

/*
 * RED test: hires_decode_scanline_color() is still page-1-only even
 * though hires_decode_scanline_mono_page() (landed in commit bab182c)
 * now supports Page 2. This is a real gap: apple2_mem_is_page2_selected()
 * is documented as driving "which decode path/page to render" for ALL of
 * Bunnie's video code, not just the mono path -- color decode should
 * follow the same PAGE1/PAGE2 selection. Proves a page-aware color
 * decode function reads its high-bit/palette data from the correct page
 * too (not just the mono on/off bits).
 */

static int test_page2_color_decode_reads_from_0x4000_base(void) {
    apple2_mem_reset();

    /* Page 1 row 0 byte 0: isolated lit pixel, even col, palette 0 ->
     * would decode GREEN if (bug) the color path ignored page2 and read
     * page 1 instead. Page 2 row 0 byte 0: high bit set + bit0 set ->
     * ORANGE if page 2 is read correctly. */
    write6502(0x2000, 0x01); /* page 1: would be GREEN if wrongly read */
    write6502(0x4000, 0x81); /* page 2: ORANGE (high bit + col0/even) */

    hires_color_t out_colors[HIRES_PIXELS_WIDE];
    hires_decode_scanline_color_page(0, /*page2=*/1, read6502, out_colors);

    if (out_colors[0] != HIRES_COLOR_ORANGE) {
        fprintf(stderr, "FAIL: page2 color[0] = %d, expected HIRES_COLOR_ORANGE (%d)\n",
                out_colors[0], HIRES_COLOR_ORANGE);
        return 1;
    }
    printf("PASS: test_page2_color_decode_reads_from_0x4000_base\n");
    return 0;
}

static int test_page1_color_decode_unaffected_by_page2_data(void) {
    apple2_mem_reset();

    write6502(0x2000, 0x01); /* page 1: GREEN (col0/even, palette 0) */
    write6502(0x4000, 0x81); /* page 2: ORANGE -- must not leak into page1 read */

    hires_color_t out_colors[HIRES_PIXELS_WIDE];
    hires_decode_scanline_color_page(0, /*page2=*/0, read6502, out_colors);

    if (out_colors[0] != HIRES_COLOR_GREEN) {
        fprintf(stderr, "FAIL: page1 color[0] = %d, expected HIRES_COLOR_GREEN (%d)\n",
                out_colors[0], HIRES_COLOR_GREEN);
        return 1;
    }
    printf("PASS: test_page1_color_decode_unaffected_by_page2_data\n");
    return 0;
}

static int test_existing_hires_decode_scanline_color_still_targets_page1(void) {
    /* Backward-compat regression guard: the original
     * hires_decode_scanline_color() (used by bio_display.c and its own
     * tests) must keep behaving exactly as before -- always page 1. */
    apple2_mem_reset();
    write6502(0x2000, 0x01); /* GREEN */

    hires_color_t out_colors[HIRES_PIXELS_WIDE];
    hires_decode_scanline_color(0, read6502, out_colors);

    if (out_colors[0] != HIRES_COLOR_GREEN) {
        fprintf(stderr,
                "FAIL: legacy hires_decode_scanline_color color[0] = %d, expected GREEN (%d)\n",
                out_colors[0], HIRES_COLOR_GREEN);
        return 1;
    }
    printf("PASS: test_existing_hires_decode_scanline_color_still_targets_page1\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_page2_color_decode_reads_from_0x4000_base();
    failures += test_page1_color_decode_unaffected_by_page2_data();
    failures += test_existing_hires_decode_scanline_color_still_targets_page1();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
