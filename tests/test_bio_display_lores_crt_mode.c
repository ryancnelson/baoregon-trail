/*
 * tests/test_bio_display_lores_crt_mode.c -- RED-first test for a real
 * gap found while reviewing Step 9's CRT monochrome modes (Green
 * Phosphor / Amber): bio_display_render_lores_frame()/
 * bio_display_render_lores_frame_mixed() call lores_color_to_rgb565()
 * directly, bypassing bio_display_color_to_rgb565()'s CRT-mode tinting
 * entirely. Hi-Res (bio_display_render_frame*()) and TEXT mode
 * (text_apple2_render_frame(), via bio_display_color_to_rgb565()) both
 * correctly re-tint under Green Phosphor/Amber CRT mode; Lo-Res
 * silently doesn't, a real visible inconsistency (switching CRT mode
 * would leave Lo-Res graphics in full color while everything else goes
 * monochrome).
 */
#include <stdio.h>
#include <string.h>

#include "../src/bio_display.h"
#include "../src/lores_apple2.h"

static uint8_t g_mock_lores_page1[1024];

static uint8_t mock_read6502(uint16_t address) {
    if (address >= LORES_PAGE1_BASE_ADDR && address < LORES_PAGE1_BASE_ADDR + sizeof(g_mock_lores_page1)) {
        return g_mock_lores_page1[address - LORES_PAGE1_BASE_ADDR];
    }
    return 0x00;
}

static int test_lores_frame_respects_green_phosphor_crt_mode(void) {
    /* Color index 0x0F ("White" in Lo-Res's palette, per
     * lores_color_to_rgb565()'s convention) must render as pure green
     * (no red/blue channel bits) once Green Phosphor CRT mode is
     * selected -- exactly the same transform bio_display_color_to_rgb565()
     * already applies to Hi-Res/TEXT output. */
    memset(g_mock_lores_page1, 0xFF, sizeof(g_mock_lores_page1)); /* both nibbles 0xF -> White */

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer));

    bio_display_set_crt_mode(BIO_CRT_MODE_GREEN_PHOSPHOR);
    bio_display_render_lores_frame(/*page2=*/0, mock_read6502, framebuffer);
    bio_display_set_crt_mode(BIO_CRT_MODE_COLOR); /* restore default for other tests */

    uint16_t px = framebuffer[0];
    if ((px & 0xF81F) != 0 || (px & 0x07E0) == 0) {
        fprintf(stderr, "FAIL: Lo-Res White pixel under Green Phosphor CRT mode should be pure "
                        "green (no R/B channel bits), got 0x%04X -- Lo-Res is bypassing CRT tinting\n", px);
        return 1;
    }
    printf("PASS: test_lores_frame_respects_green_phosphor_crt_mode\n");
    return 0;
}

static int test_lores_frame_respects_amber_crt_mode(void) {
    memset(g_mock_lores_page1, 0xFF, sizeof(g_mock_lores_page1));

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer));

    bio_display_set_crt_mode(BIO_CRT_MODE_AMBER);
    bio_display_render_lores_frame(/*page2=*/0, mock_read6502, framebuffer);
    bio_display_set_crt_mode(BIO_CRT_MODE_COLOR);

    uint16_t px = framebuffer[0];
    if ((px & 0x001F) != 0 || (px & 0xF800) == 0) {
        fprintf(stderr, "FAIL: Lo-Res White pixel under Amber CRT mode should have red/green, "
                        "zero blue, got 0x%04X -- Lo-Res is bypassing CRT tinting\n", px);
        return 1;
    }
    printf("PASS: test_lores_frame_respects_amber_crt_mode\n");
    return 0;
}

static int test_lores_frame_mixed_respects_crt_mode(void) {
    /* Same gap, via the MIXED-mode variant -- exercised separately
     * since it's a different code path (bio_display_render_lores_frame_mixed()
     * has its own duplicated rendering loop, same bug class as the
     * emulator_loop.c init()/reset_to_splash() duplication precedent
     * this session already caught once). */
    memset(g_mock_lores_page1, 0xFF, sizeof(g_mock_lores_page1));

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer));

    bio_display_set_crt_mode(BIO_CRT_MODE_GREEN_PHOSPHOR);
    bio_display_render_lores_frame_mixed(/*page2=*/0, /*mixed_mode=*/0, mock_read6502, framebuffer);
    bio_display_set_crt_mode(BIO_CRT_MODE_COLOR);

    uint16_t px = framebuffer[0];
    if ((px & 0xF81F) != 0 || (px & 0x07E0) == 0) {
        fprintf(stderr, "FAIL: Lo-Res (mixed variant) White pixel under Green Phosphor CRT mode "
                        "should be pure green, got 0x%04X\n", px);
        return 1;
    }
    printf("PASS: test_lores_frame_mixed_respects_crt_mode\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_lores_frame_respects_green_phosphor_crt_mode();
    failures += test_lores_frame_respects_amber_crt_mode();
    failures += test_lores_frame_mixed_respects_crt_mode();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
