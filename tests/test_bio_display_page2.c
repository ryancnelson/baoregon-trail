#include <stdio.h>
#include <string.h>

#include "../src/bio_display.h"
#include "../src/video_apple2.h"

/*
 * RED test: bio_display_render_frame() is still page-1-only even though
 * hires_decode_scanline_color_page() now supports Page 2. Real gap: the
 * BIO Core 0 render loop needs to honor apple2_mem_is_page2_selected()
 * just like the lower-level decode functions already do.
 */

static uint8_t g_mock_page1[8192];
static uint8_t g_mock_page2[8192];

static uint8_t mock_read6502(uint16_t address) {
    if (address >= HIRES_PAGE2_BASE_ADDR &&
        address < HIRES_PAGE2_BASE_ADDR + sizeof(g_mock_page2)) {
        return g_mock_page2[address - HIRES_PAGE2_BASE_ADDR];
    }
    uint16_t offset = address - HIRES_BASE_ADDR;
    if (offset < sizeof(g_mock_page1)) {
        return g_mock_page1[offset];
    }
    return 0x00;
}

static int test_render_frame_page_renders_page2_when_requested(void) {
    memset(g_mock_page1, 0x00, sizeof(g_mock_page1));
    memset(g_mock_page2, 0x00, sizeof(g_mock_page2));
    g_mock_page1[0] = 0x01; /* page 1: GREEN -- must NOT show up if page2=1 */
    g_mock_page2[0] = 0x81; /* page 2: ORANGE (high bit + col0/even) */

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer));

    bio_display_render_frame_page(/*page2=*/1, mock_read6502, framebuffer);

    uint16_t expected_orange = bio_display_color_to_rgb565(HIRES_COLOR_ORANGE);
    if (framebuffer[0] != expected_orange) {
        fprintf(stderr, "FAIL: framebuffer[0] = 0x%04X, expected 0x%04X (ORANGE, page2)\n",
                framebuffer[0], expected_orange);
        return 1;
    }
    printf("PASS: test_render_frame_page_renders_page2_when_requested\n");
    return 0;
}

static int test_render_frame_page_renders_page1_when_requested(void) {
    memset(g_mock_page1, 0x00, sizeof(g_mock_page1));
    memset(g_mock_page2, 0x00, sizeof(g_mock_page2));
    g_mock_page1[0] = 0x01; /* page 1: GREEN */
    g_mock_page2[0] = 0x81; /* page 2: ORANGE -- must NOT show up if page2=0 */

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer));

    bio_display_render_frame_page(/*page2=*/0, mock_read6502, framebuffer);

    uint16_t expected_green = bio_display_color_to_rgb565(HIRES_COLOR_GREEN);
    if (framebuffer[0] != expected_green) {
        fprintf(stderr, "FAIL: framebuffer[0] = 0x%04X, expected 0x%04X (GREEN, page1)\n",
                framebuffer[0], expected_green);
        return 1;
    }
    printf("PASS: test_render_frame_page_renders_page1_when_requested\n");
    return 0;
}

static int test_existing_render_frame_still_targets_page1(void) {
    /* Backward-compat regression guard. */
    memset(g_mock_page1, 0x00, sizeof(g_mock_page1));
    memset(g_mock_page2, 0x00, sizeof(g_mock_page2));
    g_mock_page1[0] = 0x01; /* GREEN */

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer));

    bio_display_render_frame(mock_read6502, framebuffer);

    uint16_t expected_green = bio_display_color_to_rgb565(HIRES_COLOR_GREEN);
    if (framebuffer[0] != expected_green) {
        fprintf(stderr,
                "FAIL: legacy bio_display_render_frame framebuffer[0] = 0x%04X, expected 0x%04X\n",
                framebuffer[0], expected_green);
        return 1;
    }
    printf("PASS: test_existing_render_frame_still_targets_page1\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_render_frame_page_renders_page2_when_requested();
    failures += test_render_frame_page_renders_page1_when_requested();
    failures += test_existing_render_frame_still_targets_page1();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
