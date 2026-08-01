#include <stdio.h>
#include <string.h>

#include "../src/bio_display.h"
#include "../src/video_apple2.h"

/*
 * RED test: MIXED mode graphics/text boundary. Real Apple II MIXED mode
 * ($C053) always shows the bottom 4 text rows (32 scanlines, rows
 * 160-191) as TEXT regardless of HIRES/LORES -- only rows 0-159 show
 * graphics. bio_display_render_frame_mixed() must stop decoding graphics
 * at row 160 when mixed_mode is set, and leave rows 160-191 untouched
 * (owned by a future text-mode renderer, not this module).
 */

static uint8_t g_mock_page1[8192];

static uint8_t mock_read6502(uint16_t address) {
    uint16_t offset = address - HIRES_BASE_ADDR;
    if (offset < sizeof(g_mock_page1)) {
        return g_mock_page1[offset];
    }
    return 0x00;
}

static int test_mixed_mode_renders_graphics_for_top_160_rows(void) {
    memset(g_mock_page1, 0x01, sizeof(g_mock_page1)); /* every byte: col0 lit -> GREEN */

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer));

    bio_display_render_frame_mixed(/*page2=*/0, /*mixed_mode=*/1, mock_read6502, framebuffer);

    uint16_t expected_green = bio_display_color_to_rgb565(HIRES_COLOR_GREEN);
    for (int row = 0; row < HIRES_MIXED_MODE_GRAPHICS_ROWS; row++) {
        uint16_t got = framebuffer[row * BIO_DISPLAY_WIDTH + 0];
        if (got != expected_green) {
            fprintf(stderr, "FAIL: row %d col0 = 0x%04X, expected 0x%04X (GREEN)\n",
                    row, got, expected_green);
            return 1;
        }
    }
    printf("PASS: test_mixed_mode_renders_graphics_for_top_160_rows\n");
    return 0;
}

static int test_mixed_mode_leaves_bottom_32_rows_untouched(void) {
    memset(g_mock_page1, 0x01, sizeof(g_mock_page1));

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer)); /* poison -- must survive untouched */

    bio_display_render_frame_mixed(/*page2=*/0, /*mixed_mode=*/1, mock_read6502, framebuffer);

    for (int row = HIRES_MIXED_MODE_GRAPHICS_ROWS; row < BIO_DISPLAY_HEIGHT; row++) {
        uint16_t got = framebuffer[row * BIO_DISPLAY_WIDTH + 0];
        if (got != 0xAAAA) {
            fprintf(stderr,
                    "FAIL: row %d col0 = 0x%04X, expected untouched poison 0xAAAA "
                    "(mixed-mode text region was overwritten by graphics decode)\n",
                    row, got);
            return 1;
        }
    }
    printf("PASS: test_mixed_mode_leaves_bottom_32_rows_untouched\n");
    return 0;
}

static int test_non_mixed_mode_renders_all_192_rows(void) {
    memset(g_mock_page1, 0x01, sizeof(g_mock_page1));

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer));

    bio_display_render_frame_mixed(/*page2=*/0, /*mixed_mode=*/0, mock_read6502, framebuffer);

    uint16_t expected_green = bio_display_color_to_rgb565(HIRES_COLOR_GREEN);
    for (int row = 0; row < BIO_DISPLAY_HEIGHT; row++) {
        uint16_t got = framebuffer[row * BIO_DISPLAY_WIDTH + 0];
        if (got != expected_green) {
            fprintf(stderr,
                    "FAIL: non-mixed row %d col0 = 0x%04X, expected 0x%04X (GREEN, full frame)\n",
                    row, got, expected_green);
            return 1;
        }
    }
    printf("PASS: test_non_mixed_mode_renders_all_192_rows\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_mixed_mode_renders_graphics_for_top_160_rows();
    failures += test_mixed_mode_leaves_bottom_32_rows_untouched();
    failures += test_non_mixed_mode_renders_all_192_rows();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
