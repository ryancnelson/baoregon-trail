#include <stdio.h>
#include <string.h>

#include "../src/bio_display.h"
#include "../src/video_apple2.h"
#include "../src/lores_apple2.h"

/*
 * RED test: real gap -- nothing in bio_display.c ever consumed
 * apple2_mem_is_hires_mode(), so the render pipeline had no way to
 * actually render a Lo-Res screen or switch between Hi-Res/Lo-Res based
 * on the current soft-switch state. Proves:
 *   1. bio_display_render_lores_frame() expands each 7x4 Lo-Res block
 *      into the native 280x192 framebuffer correctly.
 *   2. bio_display_render_frame_auto() picks HIRES vs LORES rendering
 *      based on the hires_mode flag.
 */

static uint8_t g_mock_hires_page1[8192];
static uint8_t g_mock_lores_page1[1024];

static uint8_t mock_read6502(uint16_t address) {
    if (address >= LORES_PAGE1_BASE_ADDR && address < LORES_PAGE1_BASE_ADDR + sizeof(g_mock_lores_page1)) {
        return g_mock_lores_page1[address - LORES_PAGE1_BASE_ADDR];
    }
    if (address >= HIRES_BASE_ADDR && address < HIRES_BASE_ADDR + sizeof(g_mock_hires_page1)) {
        return g_mock_hires_page1[address - HIRES_BASE_ADDR];
    }
    return 0x00;
}

static int test_lores_frame_expands_block_to_its_7x4_footprint(void) {
    memset(g_mock_lores_page1, 0x00, sizeof(g_mock_lores_page1));
    g_mock_lores_page1[0] = 0x0F; /* byte-row0 col0: top nibble=0xF (White), bottom=0x0 (Black) */

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer));

    bio_display_render_lores_frame(0, mock_read6502, framebuffer);

    uint16_t expected_white = lores_color_to_rgb565(0x0F);
    /* Top block (visible block row 0) covers pixel rows 0-3, cols 0-6. */
    for (int py = 0; py < 4; py++) {
        for (int px = 0; px < 7; px++) {
            uint16_t got = framebuffer[py * BIO_DISPLAY_WIDTH + px];
            if (got != expected_white) {
                fprintf(stderr, "FAIL: top-block pixel (%d,%d) = 0x%04X, expected 0x%04X (White)\n",
                        px, py, got, expected_white);
                return 1;
            }
        }
    }
    printf("PASS: test_lores_frame_expands_block_to_its_7x4_footprint\n");
    return 0;
}

static int test_lores_frame_bottom_nibble_covers_next_block_row(void) {
    memset(g_mock_lores_page1, 0x00, sizeof(g_mock_lores_page1));
    g_mock_lores_page1[0] = 0x0F; /* bottom nibble 0x0 (Black) */

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    bio_display_render_lores_frame(0, mock_read6502, framebuffer);

    uint16_t expected_black = lores_color_to_rgb565(0x00);
    /* Bottom block (visible block row 1) covers pixel rows 4-7, cols 0-6. */
    uint16_t got = framebuffer[4 * BIO_DISPLAY_WIDTH + 0];
    if (got != expected_black) {
        fprintf(stderr, "FAIL: bottom-block pixel (0,4) = 0x%04X, expected 0x%04X (Black)\n",
                got, expected_black);
        return 1;
    }
    printf("PASS: test_lores_frame_bottom_nibble_covers_next_block_row\n");
    return 0;
}

static int test_render_frame_auto_picks_hires_when_hires_mode_set(void) {
    memset(g_mock_hires_page1, 0x00, sizeof(g_mock_hires_page1));
    memset(g_mock_lores_page1, 0x00, sizeof(g_mock_lores_page1));
    g_mock_hires_page1[0] = 0x01; /* Hi-Res: GREEN at col0 */
    g_mock_lores_page1[0] = 0xFF; /* Lo-Res: would be White if wrongly picked */

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    bio_display_render_frame_auto(/*hires_mode=*/1, /*page2=*/0, /*mixed_mode=*/0,
                                   mock_read6502, framebuffer);

    uint16_t expected_green = bio_display_color_to_rgb565(HIRES_COLOR_GREEN);
    if (framebuffer[0] != expected_green) {
        fprintf(stderr, "FAIL: auto(hires=1) framebuffer[0] = 0x%04X, expected 0x%04X (GREEN)\n",
                framebuffer[0], expected_green);
        return 1;
    }
    printf("PASS: test_render_frame_auto_picks_hires_when_hires_mode_set\n");
    return 0;
}

static int test_render_frame_auto_picks_lores_when_hires_mode_clear(void) {
    memset(g_mock_hires_page1, 0x00, sizeof(g_mock_hires_page1));
    memset(g_mock_lores_page1, 0x00, sizeof(g_mock_lores_page1));
    g_mock_hires_page1[0] = 0x01; /* Hi-Res: would be GREEN if wrongly picked */
    g_mock_lores_page1[0] = 0x0F; /* Lo-Res: White (top nibble) */

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    bio_display_render_frame_auto(/*hires_mode=*/0, /*page2=*/0, /*mixed_mode=*/0,
                                   mock_read6502, framebuffer);

    uint16_t expected_white = lores_color_to_rgb565(0x0F);
    if (framebuffer[0] != expected_white) {
        fprintf(stderr, "FAIL: auto(hires=0) framebuffer[0] = 0x%04X, expected 0x%04X (White)\n",
                framebuffer[0], expected_white);
        return 1;
    }
    printf("PASS: test_render_frame_auto_picks_lores_when_hires_mode_clear\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_lores_frame_expands_block_to_its_7x4_footprint();
    failures += test_lores_frame_bottom_nibble_covers_next_block_row();
    failures += test_render_frame_auto_picks_hires_when_hires_mode_set();
    failures += test_render_frame_auto_picks_lores_when_hires_mode_clear();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
