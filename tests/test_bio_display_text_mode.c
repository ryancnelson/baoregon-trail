#include <stdio.h>
#include <string.h>

#include "../src/bio_display.h"
#include "../src/video_apple2.h"
#include "../src/lores_apple2.h"
#include "../src/text_apple2.h"

/*
 * Historical note (2026-08-02 UPDATE): this test file originally
 * documented and verified a RED-then-GREEN fix where full TEXT mode
 * rendered solid BLACK, because no character-ROM renderer existed yet.
 * That gap is now closed (see src/text_apple2.c, ported from the real
 * 342-0133-a.chr Apple IIe character generator ROM) -- full TEXT mode
 * now renders REAL character glyphs via text_apple2_render_frame(),
 * not a black fill. The two tests below that used to assert "always
 * black" have been updated to assert the real, correct glyph output
 * instead (verified against the actual ROM bytes, not just "not
 * black") -- this is a genuine behavior change, not a regression: the
 * old assertion was only ever a placeholder for "no renderer exists
 * yet", and text_apple2.c's own test suite (tests/test_text_apple2.c)
 * is the authoritative test for the glyph-rendering algorithm itself.
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

static int test_full_text_mode_renders_real_glyphs_not_stale_graphics(void) {
    /* Non-zero Lo-Res/Hi-Res content present -- if the renderer
     * incorrectly draws GRAPHICS in TEXT mode, this would show up as
     * content outside the real glyph-rendering path. mock_lores_page1
     * is memset to 0xFF, and TEXT and Lo-Res share the SAME memory
     * region on real Apple II hardware (see lores_apple2.h) -- so
     * mock_read6502(0x0400) genuinely returns 0xFF here, not 0x00.
     * text_apple2_decode_glyph(0xFF) decodes to a specific, known,
     * non-uniform glyph pattern (ROM index 0x7F, normal video, NOT
     * inverted since 0xFF >= 0x40) -- NOT solid black, and this test
     * asserts that EXACT real pattern rather than a vague "not
     * graphics" check, so a future regression back to black-fill (or
     * to leftover Hi-Res/Lo-Res graphics) is caught either way. */
    memset(mock_lores_page1, 0xFF, sizeof(mock_lores_page1));
    memset(mock_hires_page1, 0x01, sizeof(mock_hires_page1));

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer)); /* poison */

    bio_display_render_frame_auto_text_aware(/*hires_mode=*/0, /*page2=*/0, /*mixed_mode=*/0,
                                              /*text_mode=*/1, mock_read6502, framebuffer);

    uint8_t expected_glyph[TEXT_APPLE2_CHAR_WIDTH_PX * TEXT_APPLE2_CHAR_HEIGHT_PX];
    text_apple2_decode_glyph(0xFF, expected_glyph); /* mock's real return value for text-page reads (overlaps mock_lores_page1) */
    uint16_t white = bio_display_color_to_rgb565(HIRES_COLOR_WHITE);
    uint16_t black = bio_display_color_to_rgb565(HIRES_COLOR_BLACK);

    for (int row = 0; row < TEXT_APPLE2_CHAR_HEIGHT_PX; row++) {
        for (int col = 0; col < TEXT_APPLE2_CHAR_WIDTH_PX; col++) {
            uint16_t expected = expected_glyph[row * TEXT_APPLE2_CHAR_WIDTH_PX + col] ? white : black;
            uint16_t actual = framebuffer[row * BIO_DISPLAY_WIDTH + col];
            if (actual != expected) {
                fprintf(stderr, "FAIL: framebuffer[row=%d,col=%d] = 0x%04X, expected 0x%04X "
                                "(real character-ROM glyph for screen_byte=0xFF, not black/graphics)\n",
                        row, col, actual, expected);
                return 1;
            }
        }
    }
    printf("PASS: test_full_text_mode_renders_real_glyphs_not_stale_graphics\n");
    return 0;
}

static int test_text_mode_hires_variant_also_renders_glyphs(void) {
    /* This variant does NOT touch mock_lores_page1 (leaves it at
     * whatever the previous test left it, but each test function here
     * re-memsets what it needs) -- mock_hires_page1 alone is set, and
     * mock_lores_page1 is untouched by THIS function, so its content
     * depends on prior test execution order. To keep this test
     * self-contained and not order-dependent, explicitly reset
     * mock_lores_page1 to zero (real screen_byte 0x00) here. */
    memset(mock_lores_page1, 0x00, sizeof(mock_lores_page1));
    memset(mock_hires_page1, 0x01, sizeof(mock_hires_page1));

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer));

    bio_display_render_frame_auto_text_aware(/*hires_mode=*/1, /*page2=*/0, /*mixed_mode=*/0,
                                              /*text_mode=*/1, mock_read6502, framebuffer);

    uint8_t expected_glyph[TEXT_APPLE2_CHAR_WIDTH_PX * TEXT_APPLE2_CHAR_HEIGHT_PX];
    text_apple2_decode_glyph(0x00, expected_glyph);
    uint16_t white = bio_display_color_to_rgb565(HIRES_COLOR_WHITE);
    uint16_t black = bio_display_color_to_rgb565(HIRES_COLOR_BLACK);
    uint16_t expected0 = expected_glyph[0] ? white : black;

    if (framebuffer[0] != expected0) {
        fprintf(stderr, "FAIL: framebuffer[0] = 0x%04X, expected 0x%04X -- "
                        "TEXT mode with hires_mode=1 must still render real glyphs "
                        "(hires_mode is irrelevant once text_mode is set)\n", framebuffer[0], expected0);
        return 1;
    }
    printf("PASS: test_text_mode_hires_variant_also_renders_glyphs\n");
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
    failures += test_full_text_mode_renders_real_glyphs_not_stale_graphics();
    failures += test_text_mode_hires_variant_also_renders_glyphs();
    failures += test_mixed_mode_overrides_text_mode_and_renders_graphics();
    failures += test_graphics_mode_renders_normally();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
