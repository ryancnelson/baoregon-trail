#include <stdio.h>
#include <string.h>

#include "../src/video_apple2.h"
#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"

/*
 * RED test: Hi-Res Page 2 support. Real Apple II hardware has TWO Hi-Res
 * pages -- Page 1 at $2000-$3FFF (what every existing video_apple2 test
 * exercises) and Page 2 at $4000-$5FFF, selected via the $C054/$C055
 * PAGE1/PAGE2 soft-switch that apple2_mem.c now implements (landed by the
 * team since my last check-in). video_apple2.c currently hardcodes
 * HIRES_BASE_ADDR ($2000, page 1 only) with no way to render page 2 --
 * a real gap now that apple2_mem_is_page2_selected() exists and is meant
 * to drive which page Bunnie's code renders (see apple2_mem.h's own doc
 * comment: "read by Bunnie's video_apple2.c to pick which decode
 * path/page to render").
 *
 * This test proves a page-aware decode path reads from the CORRECT base
 * address for each page, using apple2_mem.c's real read6502/write6502
 * bus (not a mock) so the page-select soft-switch state is exercised
 * end-to-end, matching the pattern already used in
 * test_video_apple2_realbus.c.
 */

static int test_page1_decode_reads_from_0x2000_base(void) {
    apple2_mem_reset();

    write6502(0x2000, 0x55); /* page 1 row 0 byte 0 */

    uint8_t out_pixels[HIRES_PIXELS_WIDE];
    hires_decode_scanline_mono_page(0, /*page2=*/0, read6502, out_pixels);

    const uint8_t expected_first7[7] = {1, 0, 1, 0, 1, 0, 1};
    for (int i = 0; i < 7; i++) {
        if (out_pixels[i] != expected_first7[i]) {
            fprintf(stderr, "FAIL: page1 pixel[%d] = %u, expected %u\n",
                    i, out_pixels[i], expected_first7[i]);
            return 1;
        }
    }
    printf("PASS: test_page1_decode_reads_from_0x2000_base\n");
    return 0;
}

static int test_page2_decode_reads_from_0x4000_base(void) {
    apple2_mem_reset();

    /* Page 2 lives at $4000-$5FFF -- same 192-row interleave, just offset
     * by 0x2000 from page 1's base per real Apple II hardware. Write to
     * $4000 (page 2 row 0 byte 0), leave $2000 (page 1) untouched. */
    write6502(0x4000, 0x2A); /* 0b0010_1010 -> bits 0..6 = 0,1,0,1,0,1,0 */

    uint8_t out_pixels[HIRES_PIXELS_WIDE];
    hires_decode_scanline_mono_page(0, /*page2=*/1, read6502, out_pixels);

    const uint8_t expected_first7[7] = {0, 1, 0, 1, 0, 1, 0};
    for (int i = 0; i < 7; i++) {
        if (out_pixels[i] != expected_first7[i]) {
            fprintf(stderr, "FAIL: page2 pixel[%d] = %u, expected %u\n",
                    i, out_pixels[i], expected_first7[i]);
            return 1;
        }
    }
    printf("PASS: test_page2_decode_reads_from_0x4000_base\n");
    return 0;
}

static int test_page1_and_page2_are_independent_buffers(void) {
    apple2_mem_reset();

    /* Write different data to page 1 and page 2 row 0 byte 0; decoding
     * page 1 must NOT see page 2's data and vice versa -- proves the two
     * pages don't alias into each other's address range. */
    write6502(0x2000, 0x7F); /* page 1: all 7 pixels lit */
    write6502(0x4000, 0x00); /* page 2: all 7 pixels dark */

    uint8_t page1_pixels[HIRES_PIXELS_WIDE];
    uint8_t page2_pixels[HIRES_PIXELS_WIDE];
    hires_decode_scanline_mono_page(0, 0, read6502, page1_pixels);
    hires_decode_scanline_mono_page(0, 1, read6502, page2_pixels);

    for (int i = 0; i < 7; i++) {
        if (page1_pixels[i] != 1) {
            fprintf(stderr, "FAIL: page1 pixel[%d] = %u, expected 1\n", i, page1_pixels[i]);
            return 1;
        }
        if (page2_pixels[i] != 0) {
            fprintf(stderr, "FAIL: page2 pixel[%d] = %u, expected 0\n", i, page2_pixels[i]);
            return 1;
        }
    }
    printf("PASS: test_page1_and_page2_are_independent_buffers\n");
    return 0;
}

static int test_existing_hires_decode_scanline_mono_still_targets_page1(void) {
    /* Backward-compat: the original hires_decode_scanline_mono() (used by
     * every pre-existing test and by bio_display.c) must keep behaving
     * exactly as before -- always page 1, no signature change. This is a
     * regression guard for the refactor that introduces the page-aware
     * variant. */
    apple2_mem_reset();
    write6502(0x2000, 0x55);

    uint8_t out_pixels[HIRES_PIXELS_WIDE];
    hires_decode_scanline_mono(0, read6502, out_pixels);

    const uint8_t expected_first7[7] = {1, 0, 1, 0, 1, 0, 1};
    for (int i = 0; i < 7; i++) {
        if (out_pixels[i] != expected_first7[i]) {
            fprintf(stderr,
                    "FAIL: legacy hires_decode_scanline_mono pixel[%d] = %u, expected %u\n",
                    i, out_pixels[i], expected_first7[i]);
            return 1;
        }
    }
    printf("PASS: test_existing_hires_decode_scanline_mono_still_targets_page1\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_page1_decode_reads_from_0x2000_base();
    failures += test_page2_decode_reads_from_0x4000_base();
    failures += test_page1_and_page2_are_independent_buffers();
    failures += test_existing_hires_decode_scanline_mono_still_targets_page1();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
