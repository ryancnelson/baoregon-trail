#include <stdio.h>
#include <string.h>

#include "../src/bio_display.h"
#include "../src/video_apple2.h"
#include "../src/lores_apple2.h"

/*
 * RED test: real bug -- bio_display_render_frame_auto() never checked
 * apple2_mem_is_text_mode(), so it always rendered Hi-Res/Lo-Res
 * graphics into the framebuffer even in full TEXT mode (the real Apple
 * II post-reset default!). bio_display_render_frame_auto_text_aware()
 * fixes this: full TEXT mode (not MIXED) must be a safe no-op.
 */

static uint8_t mock_hires_page1[8192];
static uint8_t mock_lores_page1[1024];

static uint8_t mock_read6502(uint16_t address) {
    if (address >= LORES_PAGE1_BASE_ADDR && address < LORES_PAGE1_BASE_ADDR + sizeof(mock_lores_page1)) {
        return mock_lores_page1[address - LORES_PAGE1_BASE_ADDR];
    }
    if (address >= HIRES_BASE_ADDR && address < HIRES_BASE_ADDR + sizeof(mock_hires_page1)) {
        return mock_hires_page1[address - HIRES_BASE_ADDR];
    }
    return 0x00;
}

static int test_full_text_mode_leaves_framebuffer_untouched(void) {
    /* Non-zero Lo-Res/Hi-Res content present -- if the renderer
     * incorrectly draws graphics in TEXT mode, this would show up. */
    memset(mock_lores_page1, 0xFF, sizeof(mock_lores_page1));
    memset(mock_hires_page1, 0x01, sizeof(mock_hires_page1));

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer)); /* poison */

    /* text_mode=1, mixed_mode=0 (full-screen TEXT) -- must be a no-op
     * regardless of hires_mode/page2. */
    bio_display_render_frame_auto_text_aware(/*hires_mode=*/0, /*page2=*/0, /*mixed_mode=*/0,
                                              /*text_mode=*/1, mock_read6502, framebuffer);

    for (int i = 0; i < BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT; i++) {
        if (framebuffer[i] != 0xAAAA) {
            fprintf(stderr, "FAIL: framebuffer[%d] = 0x%04X, expected untouched poison 0xAAAA "
                            "(full TEXT mode incorrectly rendered graphics)\n", i, framebuffer[i]);
            return 1;
        }
    }
    printf("PASS: test_full_text_mode_leaves_framebuffer_untouched\n");
    return 0;
}

static int test_text_mode_hires_variant_also_noop(void) {
    memset(mock_hires_page1, 0x01, sizeof(mock_hires_page1));

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer));

    bio_display_render_frame_auto_text_aware(/*hires_mode=*/1, /*page2=*/0, /*mixed_mode=*/0,
                                              /*text_mode=*/1, mock_read6502, framebuffer);

    if (framebuffer[0] != 0xAAAA) {
        fprintf(stderr, "FAIL: framebuffer[0] = 0x%04X, expected untouched poison -- "
                        "TEXT mode with hires_mode=1 still incorrectly rendered\n", framebuffer[0]);
        return 1;
    }
    printf("PASS: test_text_mode_hires_variant_also_noop\n");
    return 0;
}

static int test_mixed_mode_overrides_text_mode_and_renders_graphics(void) {
    /* Real Apple II: MIXED mode always shows graphics on top regardless
     * of TEXT/GRAPHICS switch state -- text_mode=1 + mixed_mode=1 must
     * still render the top 160 rows of graphics. */
    memset(mock_lores_page1, 0xFF, sizeof(mock_lores_page1));

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer));

    bio_display_render_frame_auto_text_aware(/*hires_mode=*/0, /*page2=*/0, /*mixed_mode=*/1,
                                              /*text_mode=*/1, mock_read6502, framebuffer);

    uint16_t expected_white = lores_color_to_rgb565(0x0F);
    if (framebuffer[0] != expected_white) {
        fprintf(stderr, "FAIL: framebuffer[0] = 0x%04X, expected 0x%04X (White) -- "
                        "MIXED mode should still render graphics even with text_mode=1\n",
                framebuffer[0], expected_white);
        return 1;
    }
    printf("PASS: test_mixed_mode_overrides_text_mode_and_renders_graphics\n");
    return 0;
}

static int test_graphics_mode_renders_normally(void) {
    /* Sanity: text_mode=0 (GRAPHICS) behaves identically to the
     * existing bio_display_render_frame_auto(). */
    memset(mock_lores_page1, 0xFF, sizeof(mock_lores_page1));

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer));

    bio_display_render_frame_auto_text_aware(/*hires_mode=*/0, /*page2=*/0, /*mixed_mode=*/0,
                                              /*text_mode=*/0, mock_read6502, framebuffer);

    uint16_t expected_white = lores_color_to_rgb565(0x0F);
    if (framebuffer[0] != expected_white) {
        fprintf(stderr, "FAIL: framebuffer[0] = 0x%04X, expected 0x%04X (White) -- "
                        "GRAPHICS mode (text_mode=0) should render normally\n",
                framebuffer[0], expected_white);
        return 1;
    }
    printf("PASS: test_graphics_mode_renders_normally\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_full_text_mode_leaves_framebuffer_untouched();
    failures += test_text_mode_hires_variant_also_noop();
    failures += test_mixed_mode_overrides_text_mode_and_renders_graphics();
    failures += test_graphics_mode_renders_normally();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
