#include <stdio.h>
#include <string.h>

#include "../src/video_apple2.h"

/*
 * RED test: video_apple2.c had no defensive checks for out-of-range
 * `row` or NULL `read_mem`/output-buffer pointers -- a real gap now that
 * the team has established a "safe no-op on bad input, never crash"
 * convention elsewhere (disk_trap.c's disk_trap_read_byte(),
 * bunnie_audio.c's null checks landed in commit d0035e9). An
 * out-of-range row previously read hires_line_offsets[row] out of
 * bounds (undefined behavior); a NULL read_mem previously crashed with
 * a null-pointer-call segfault.
 */

static uint8_t mock_read6502(uint16_t address) {
    (void)address;
    return 0xFF;
}

static int test_mono_out_of_range_row_is_safe_noop(void) {
    uint8_t out_pixels[HIRES_PIXELS_WIDE];
    memset(out_pixels, 0xAA, sizeof(out_pixels)); /* poison to catch any write */

    hires_decode_scanline_mono(HIRES_ROWS, mock_read6502, out_pixels); /* row == 192, out of range */
    hires_decode_scanline_mono(-1, mock_read6502, out_pixels);         /* negative row */

    for (int i = 0; i < HIRES_PIXELS_WIDE; i++) {
        if (out_pixels[i] != 0xAA) {
            fprintf(stderr, "FAIL: out_pixels[%d] = %u, expected untouched poison 0xAA "
                            "(out-of-range row was not a safe no-op)\n", i, out_pixels[i]);
            return 1;
        }
    }
    printf("PASS: test_mono_out_of_range_row_is_safe_noop\n");
    return 0;
}

static int test_mono_null_read_mem_is_safe_noop(void) {
    uint8_t out_pixels[HIRES_PIXELS_WIDE];
    memset(out_pixels, 0xAA, sizeof(out_pixels));

    hires_decode_scanline_mono(0, NULL, out_pixels); /* must not segfault */

    for (int i = 0; i < HIRES_PIXELS_WIDE; i++) {
        if (out_pixels[i] != 0xAA) {
            fprintf(stderr, "FAIL: out_pixels[%d] = %u, expected untouched poison 0xAA "
                            "(NULL read_mem was not a safe no-op)\n", i, out_pixels[i]);
            return 1;
        }
    }
    printf("PASS: test_mono_null_read_mem_is_safe_noop\n");
    return 0;
}

static int test_mono_null_out_pixels_is_safe_noop(void) {
    /* Must not segfault when out_pixels is NULL. */
    hires_decode_scanline_mono(0, mock_read6502, NULL);
    printf("PASS: test_mono_null_out_pixels_is_safe_noop\n");
    return 0;
}

static int test_color_out_of_range_row_is_safe_noop(void) {
    hires_color_t out_colors[HIRES_PIXELS_WIDE];
    for (int i = 0; i < HIRES_PIXELS_WIDE; i++) {
        out_colors[i] = (hires_color_t)0xFF; /* poison (not a valid enum value) */
    }

    hires_decode_scanline_color(HIRES_ROWS, mock_read6502, out_colors);
    hires_decode_scanline_color(-1, mock_read6502, out_colors);

    for (int i = 0; i < HIRES_PIXELS_WIDE; i++) {
        if (out_colors[i] != (hires_color_t)0xFF) {
            fprintf(stderr, "FAIL: out_colors[%d] = %d, expected untouched poison "
                            "(out-of-range row was not a safe no-op)\n", i, out_colors[i]);
            return 1;
        }
    }
    printf("PASS: test_color_out_of_range_row_is_safe_noop\n");
    return 0;
}

static int test_color_null_pointers_are_safe_noop(void) {
    hires_color_t out_colors[HIRES_PIXELS_WIDE];
    hires_decode_scanline_color(0, NULL, out_colors); /* NULL read_mem */
    hires_decode_scanline_color(0, mock_read6502, NULL); /* NULL out_colors */
    printf("PASS: test_color_null_pointers_are_safe_noop\n");
    return 0;
}

static int test_page_variants_reject_bad_input_too(void) {
    uint8_t out_pixels[HIRES_PIXELS_WIDE];
    memset(out_pixels, 0xAA, sizeof(out_pixels));
    hires_decode_scanline_mono_page(HIRES_ROWS, 0, mock_read6502, out_pixels);
    hires_decode_scanline_mono_page(0, 0, NULL, out_pixels);
    hires_decode_scanline_mono_page(0, 0, mock_read6502, NULL);

    hires_color_t out_colors[HIRES_PIXELS_WIDE];
    hires_decode_scanline_color_page(HIRES_ROWS, 0, mock_read6502, out_colors);
    hires_decode_scanline_color_page(0, 0, NULL, out_colors);
    hires_decode_scanline_color_page(0, 0, mock_read6502, NULL);

    for (int i = 0; i < HIRES_PIXELS_WIDE; i++) {
        if (out_pixels[i] != 0xAA) {
            fprintf(stderr, "FAIL: page-variant mono out_pixels[%d] = %u, expected untouched 0xAA\n",
                    i, out_pixels[i]);
            return 1;
        }
    }
    printf("PASS: test_page_variants_reject_bad_input_too\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_mono_out_of_range_row_is_safe_noop();
    failures += test_mono_null_read_mem_is_safe_noop();
    failures += test_mono_null_out_pixels_is_safe_noop();
    failures += test_color_out_of_range_row_is_safe_noop();
    failures += test_color_null_pointers_are_safe_noop();
    failures += test_page_variants_reject_bad_input_too();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
