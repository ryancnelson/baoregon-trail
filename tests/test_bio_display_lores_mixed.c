#include <stdio.h>
#include <string.h>

#include "../src/bio_display.h"
#include "../src/lores_apple2.h"

/*
 * RED test: real gap -- Lo-Res + MIXED mode was untested and unimplemented.
 * Real Apple II MIXED mode forces the bottom 4 text rows (32 scanlines,
 * pixel rows 160-191) to TEXT regardless of HIRES/LORES -- Lo-Res's
 * graphics decode must stop at the same pixel-row boundary as Hi-Res's
 * bio_display_render_frame_mixed() already does.
 */

static uint8_t g_mock_lores_page1[1024];

static uint8_t mock_read6502(uint16_t address) {
    if (address >= LORES_PAGE1_BASE_ADDR && address < LORES_PAGE1_BASE_ADDR + sizeof(g_mock_lores_page1)) {
        return g_mock_lores_page1[address - LORES_PAGE1_BASE_ADDR];
    }
    return 0x00;
}

static int test_lores_mixed_renders_graphics_for_top_160_rows(void) {
    memset(g_mock_lores_page1, 0xFF, sizeof(g_mock_lores_page1)); /* both nibbles 0xF -> White */

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer));

    bio_display_render_lores_frame_mixed(/*page2=*/0, /*mixed_mode=*/1, mock_read6502, framebuffer);

    uint16_t expected_white = lores_color_to_rgb565(0x0F);
    for (int row = 0; row < HIRES_MIXED_MODE_GRAPHICS_ROWS; row++) {
        uint16_t got = framebuffer[row * BIO_DISPLAY_WIDTH + 0];
        if (got != expected_white) {
            fprintf(stderr, "FAIL: row %d col0 = 0x%04X, expected 0x%04X (White)\n",
                    row, got, expected_white);
            return 1;
        }
    }
    printf("PASS: test_lores_mixed_renders_graphics_for_top_160_rows\n");
    return 0;
}

static int test_lores_mixed_leaves_bottom_32_rows_untouched(void) {
    memset(g_mock_lores_page1, 0xFF, sizeof(g_mock_lores_page1));

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer)); /* poison -- must survive untouched */

    bio_display_render_lores_frame_mixed(/*page2=*/0, /*mixed_mode=*/1, mock_read6502, framebuffer);

    for (int row = HIRES_MIXED_MODE_GRAPHICS_ROWS; row < BIO_DISPLAY_HEIGHT; row++) {
        uint16_t got = framebuffer[row * BIO_DISPLAY_WIDTH + 0];
        if (got != 0xAAAA) {
            fprintf(stderr,
                    "FAIL: row %d col0 = 0x%04X, expected untouched poison 0xAAAA "
                    "(Lo-Res mixed-mode text region was overwritten by graphics decode)\n",
                    row, got);
            return 1;
        }
    }
    printf("PASS: test_lores_mixed_leaves_bottom_32_rows_untouched\n");
    return 0;
}

static int test_lores_non_mixed_renders_all_192_rows(void) {
    memset(g_mock_lores_page1, 0xFF, sizeof(g_mock_lores_page1));

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer));

    bio_display_render_lores_frame_mixed(/*page2=*/0, /*mixed_mode=*/0, mock_read6502, framebuffer);

    uint16_t expected_white = lores_color_to_rgb565(0x0F);
    for (int row = 0; row < BIO_DISPLAY_HEIGHT; row++) {
        uint16_t got = framebuffer[row * BIO_DISPLAY_WIDTH + 0];
        if (got != expected_white) {
            fprintf(stderr,
                    "FAIL: non-mixed row %d col0 = 0x%04X, expected 0x%04X (White, full frame)\n",
                    row, got, expected_white);
            return 1;
        }
    }
    printf("PASS: test_lores_non_mixed_renders_all_192_rows\n");
    return 0;
}

static int test_render_frame_auto_honors_mixed_mode_in_lores(void) {
    memset(g_mock_lores_page1, 0xFF, sizeof(g_mock_lores_page1));

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer));

    bio_display_render_frame_auto(/*hires_mode=*/0, /*page2=*/0, /*mixed_mode=*/1,
                                   mock_read6502, framebuffer);

    uint16_t got_bottom = framebuffer[HIRES_MIXED_MODE_GRAPHICS_ROWS * BIO_DISPLAY_WIDTH + 0];
    if (got_bottom != 0xAAAA) {
        fprintf(stderr,
                "FAIL: auto(hires=0,mixed=1) bottom row = 0x%04X, expected untouched poison -- "
                "render_frame_auto did not pass mixed_mode through to the Lo-Res path\n",
                got_bottom);
        return 1;
    }
    printf("PASS: test_render_frame_auto_honors_mixed_mode_in_lores\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_lores_mixed_renders_graphics_for_top_160_rows();
    failures += test_lores_mixed_leaves_bottom_32_rows_untouched();
    failures += test_lores_non_mixed_renders_all_192_rows();
    failures += test_render_frame_auto_honors_mixed_mode_in_lores();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
