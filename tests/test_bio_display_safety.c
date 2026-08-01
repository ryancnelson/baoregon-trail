#include <stdio.h>

#include "../src/bio_display.h"

/*
 * RED test: bio_display.c had no defensive checks -- real gap now that
 * the team has established a "safe no-op / safe fallback on bad input,
 * never crash" convention across the codebase (10e814c video_apple2 &
 * lores_apple2, 90b7e69 disk_sector_layout, f50a5c7 boot_splash,
 * d0035e9 bunnie_audio, our own earlier RED-verified video_apple2.c /
 * lores_apple2.c fix). bio_display_color_to_rgb565() had no bounds
 * check on an out-of-range hires_color_t (out-of-bounds array read,
 * undefined behavior); the render functions had no NULL check on
 * framebuffer (real segfault on write).
 */

static uint8_t mock_read6502(uint16_t address) {
    (void)address;
    return 0xFF;
}

static int test_color_to_rgb565_out_of_range_returns_black_fallback(void) {
    /* HIRES_COLOR_WHITE is the last valid enum value (5); anything past
     * that is out of range and must fall back to black (0x0000), same
     * convention as lores_color_to_rgb565(). */
    uint16_t got = bio_display_color_to_rgb565((hires_color_t)99);
    if (got != 0x0000) {
        fprintf(stderr, "FAIL: bio_display_color_to_rgb565(99) = 0x%04X, expected 0x0000 (black fallback)\n", got);
        return 1;
    }
    printf("PASS: test_color_to_rgb565_out_of_range_returns_black_fallback\n");
    return 0;
}

static int test_render_frame_null_framebuffer_is_safe_noop(void) {
    /* Must not segfault. */
    bio_display_render_frame(mock_read6502, NULL);
    printf("PASS: test_render_frame_null_framebuffer_is_safe_noop\n");
    return 0;
}

static int test_render_frame_page_null_framebuffer_is_safe_noop(void) {
    bio_display_render_frame_page(0, mock_read6502, NULL);
    printf("PASS: test_render_frame_page_null_framebuffer_is_safe_noop\n");
    return 0;
}

static int test_render_frame_mixed_null_framebuffer_is_safe_noop(void) {
    bio_display_render_frame_mixed(0, 0, mock_read6502, NULL);
    bio_display_render_frame_mixed(0, 1, mock_read6502, NULL);
    printf("PASS: test_render_frame_mixed_null_framebuffer_is_safe_noop\n");
    return 0;
}

static int test_render_lores_frame_null_framebuffer_is_safe_noop(void) {
    bio_display_render_lores_frame(0, mock_read6502, NULL);
    printf("PASS: test_render_lores_frame_null_framebuffer_is_safe_noop\n");
    return 0;
}

static int test_render_lores_frame_mixed_null_framebuffer_is_safe_noop(void) {
    bio_display_render_lores_frame_mixed(0, 0, mock_read6502, NULL);
    bio_display_render_lores_frame_mixed(0, 1, mock_read6502, NULL);
    printf("PASS: test_render_lores_frame_mixed_null_framebuffer_is_safe_noop\n");
    return 0;
}

static int test_render_frame_auto_null_framebuffer_is_safe_noop(void) {
    bio_display_render_frame_auto(1, 0, 0, mock_read6502, NULL);
    bio_display_render_frame_auto(0, 0, 0, mock_read6502, NULL);
    printf("PASS: test_render_frame_auto_null_framebuffer_is_safe_noop\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_color_to_rgb565_out_of_range_returns_black_fallback();
    failures += test_render_frame_null_framebuffer_is_safe_noop();
    failures += test_render_frame_page_null_framebuffer_is_safe_noop();
    failures += test_render_frame_mixed_null_framebuffer_is_safe_noop();
    failures += test_render_lores_frame_null_framebuffer_is_safe_noop();
    failures += test_render_lores_frame_mixed_null_framebuffer_is_safe_noop();
    failures += test_render_frame_auto_null_framebuffer_is_safe_noop();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
