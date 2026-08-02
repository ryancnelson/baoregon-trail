/*
 * test_text_apple2.c -- TDD tests for src/text_apple2.c's real
 * character-ROM glyph renderer (NEXT_STEPS.md priority task, 2026-08-02).
 *
 * Expected pixel patterns below are ground truth extracted directly
 * from the real, MAME-verified 342-0133-a.chr ROM bytes (SHA1
 * 7060de104046736529c1e8a687a0dd7b84f8c51b) BEFORE any C code was
 * written -- decoded via a small Python script applying the documented
 * algorithm (rom_index = byte & 0x7F, invert = byte < 0x40) and
 * visually confirmed to render a real, readable "DOS VERSION 3.3"
 * string from actual screen-memory bytes captured from a real DOS 3.3
 * boot (see src/main_qemu_disk2boot.c / tools/boot_disk2_real_dsk.c).
 * This is genuine known-input/known-output TDD, not a test written to
 * match whatever the implementation happens to produce.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../src/text_apple2.h"
#include "../src/bio_display.h"

/* Real ROM bytes at index 0x44 (rom_index for both raw ASCII 'D'=0x44
 * and Apple II normal-video 'D'=0xC4, since 0xC4 & 0x7F == 0x44):
 * 0x1E,0x22,0x22,0x22,0x22,0x22,0x1E,0x00 -- confirmed directly from
 * the real ROM file, not invented. */
static const uint8_t D_GLYPH_ROM_BYTES[8] = {0x1E, 0x22, 0x22, 0x22, 0x22, 0x22, 0x1E, 0x00};

static void expected_bits_from_rom_bytes(const uint8_t rom_bytes[8], int invert,
                                          uint8_t out[TEXT_APPLE2_CHAR_WIDTH_PX * TEXT_APPLE2_CHAR_HEIGHT_PX]) {
    for (int row = 0; row < TEXT_APPLE2_CHAR_HEIGHT_PX; row++) {
        uint8_t bits = rom_bytes[row] & 0x7F;
        if (invert) bits ^= 0x7F;
        for (int col = 0; col < TEXT_APPLE2_CHAR_WIDTH_PX; col++) {
            /* Bit 0 = leftmost pixel (fixed 2026-08-02 alongside the
             * real bug in text_apple2.c: this test helper had
             * self-consistently encoded the SAME mirror-reversed bit
             * order as the buggy implementation, so it never caught the
             * bug -- a 'D' glyph is horizontally symmetric-ish enough
             * that mirroring wasn't visually obvious; an 'F' glyph
             * (asymmetric) exposed it immediately when Ryan visually
             * inspected a real screenshot and saw mirror-reversed text). */
            out[row * TEXT_APPLE2_CHAR_WIDTH_PX + col] = (uint8_t)((bits >> col) & 1);
        }
    }
}

static void test_decode_glyph_normal_video_D(void) {
    /* screen_byte 0xC4 = Apple II "normal video" uppercase 'D'
     * (0x80-0xFF range, high-bit-set ASCII -- confirmed this is the
     * exact byte value DOS 3.3 actually writes for 'D' at $0400). */
    uint8_t actual[TEXT_APPLE2_CHAR_WIDTH_PX * TEXT_APPLE2_CHAR_HEIGHT_PX];
    text_apple2_decode_glyph(0xC4, actual);

    uint8_t expected[TEXT_APPLE2_CHAR_WIDTH_PX * TEXT_APPLE2_CHAR_HEIGHT_PX];
    expected_bits_from_rom_bytes(D_GLYPH_ROM_BYTES, 0, expected);

    assert(memcmp(actual, expected, sizeof(expected)) == 0);
    printf("PASS: test_decode_glyph_normal_video_D\n");
}

static void test_decode_glyph_inverse_video_D(void) {
    /* screen_byte 0x04 = same 'D' glyph shape, but in the 0x00-0x3F
     * INVERSE VIDEO range -- foreground/background bits must be
     * swapped relative to the normal-video case above. */
    uint8_t actual[TEXT_APPLE2_CHAR_WIDTH_PX * TEXT_APPLE2_CHAR_HEIGHT_PX];
    text_apple2_decode_glyph(0x04, actual);

    uint8_t expected[TEXT_APPLE2_CHAR_WIDTH_PX * TEXT_APPLE2_CHAR_HEIGHT_PX];
    expected_bits_from_rom_bytes(D_GLYPH_ROM_BYTES, 1, expected);

    assert(memcmp(actual, expected, sizeof(expected)) == 0);
    printf("PASS: test_decode_glyph_inverse_video_D\n");
}

static void test_decode_glyph_flash_range_renders_normal(void) {
    /* screen_byte 0x44 is in the 0x40-0x7F "flash" range -- this
     * module doesn't implement the ~2Hz flash timer (documented
     * simplification), so it must render identically to the
     * non-inverted case (same bits as 0xC4 above, NOT inverted). */
    uint8_t actual[TEXT_APPLE2_CHAR_WIDTH_PX * TEXT_APPLE2_CHAR_HEIGHT_PX];
    text_apple2_decode_glyph(0x44, actual);

    uint8_t expected[TEXT_APPLE2_CHAR_WIDTH_PX * TEXT_APPLE2_CHAR_HEIGHT_PX];
    expected_bits_from_rom_bytes(D_GLYPH_ROM_BYTES, 0, expected);

    assert(memcmp(actual, expected, sizeof(expected)) == 0);
    printf("PASS: test_decode_glyph_flash_range_renders_normal\n");
}

static void test_decode_glyph_null_safety(void) {
    /* Must not crash on NULL out_bits -- matches every other
     * renderer's safety convention in this codebase. */
    text_apple2_decode_glyph(0xC4, NULL);
    printf("PASS: test_decode_glyph_null_safety\n");
}

/* Mock 64KB Apple II address space backing read6502_fn, matching the
 * pattern used by test_lores_apple2.c / test_video_apple2.c. */
static uint8_t g_mock_ram[65536];
static uint8_t mock_read6502(uint16_t addr) {
    return g_mock_ram[addr];
}

static void test_render_frame_renders_D_at_top_left(void) {
    memset(g_mock_ram, 0xA0, sizeof(g_mock_ram)); /* 0xA0 = space, matches real DOS output's padding */
    g_mock_ram[TEXT_APPLE2_PAGE1_BASE_ADDR + 0] = 0xC4; /* 'D' at row 0, col 0 (offset 0 in the interleave table) */

    static uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xFF, sizeof(framebuffer)); /* poison: must be overwritten */

    text_apple2_render_frame(0, mock_read6502, framebuffer);

    uint8_t expected_bits[TEXT_APPLE2_CHAR_WIDTH_PX * TEXT_APPLE2_CHAR_HEIGHT_PX];
    expected_bits_from_rom_bytes(D_GLYPH_ROM_BYTES, 0, expected_bits);

    uint16_t white = bio_display_color_to_rgb565(HIRES_COLOR_WHITE);
    uint16_t black = bio_display_color_to_rgb565(HIRES_COLOR_BLACK);

    for (int row = 0; row < TEXT_APPLE2_CHAR_HEIGHT_PX; row++) {
        for (int col = 0; col < TEXT_APPLE2_CHAR_WIDTH_PX; col++) {
            uint16_t expected_color = expected_bits[row * TEXT_APPLE2_CHAR_WIDTH_PX + col] ? white : black;
            uint16_t actual_color = framebuffer[row * BIO_DISPLAY_WIDTH + col];
            assert(actual_color == expected_color);
        }
    }
    printf("PASS: test_render_frame_renders_D_at_top_left\n");
}

static void test_render_frame_row_interleave_second_row(void) {
    /* Real Apple II text/lores row interleave: byte-row 1 (the SECOND
     * on-screen row of characters) lives at memory offset 0x0080 from
     * the page base (see lores_apple2.c's lores_byte_row_offsets[1]),
     * NOT at a naive linear offset of 40. Placing 'D' there and
     * confirming it renders at PIXEL row 8 (character row 1 * 8px)
     * proves the real non-linear interleave table is actually being
     * used, not a wrong/linear assumption. */
    memset(g_mock_ram, 0xA0, sizeof(g_mock_ram));
    g_mock_ram[TEXT_APPLE2_PAGE1_BASE_ADDR + 0x0080] = 0xC4;

    static uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0, sizeof(framebuffer));

    text_apple2_render_frame(0, mock_read6502, framebuffer);

    uint16_t white = bio_display_color_to_rgb565(HIRES_COLOR_WHITE);
    /* Row 0 (pixel rows 0-7) must be all-black (space characters). */
    int row0_all_black = 1;
    for (int i = 0; i < TEXT_APPLE2_CHAR_HEIGHT_PX * BIO_DISPLAY_WIDTH; i++) {
        if (framebuffer[i] == white) { row0_all_black = 0; break; }
    }
    assert(row0_all_black);

    /* Character row 1 (pixel rows 8-15) at column 0 must show the 'D'
     * glyph's lit pixels somewhere (not all black). */
    int row1_has_white = 0;
    for (int row = TEXT_APPLE2_CHAR_HEIGHT_PX; row < TEXT_APPLE2_CHAR_HEIGHT_PX * 2; row++) {
        for (int col = 0; col < TEXT_APPLE2_CHAR_WIDTH_PX; col++) {
            if (framebuffer[row * BIO_DISPLAY_WIDTH + col] == white) { row1_has_white = 1; break; }
        }
    }
    assert(row1_has_white);
    printf("PASS: test_render_frame_row_interleave_second_row\n");
}

static void test_render_frame_null_safety(void) {
    text_apple2_render_frame(0, NULL, NULL);
    static uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    text_apple2_render_frame(0, mock_read6502, NULL);
    text_apple2_render_frame(0, NULL, framebuffer);
    printf("PASS: test_render_frame_null_safety\n");
}

int main(void) {
    test_decode_glyph_normal_video_D();
    test_decode_glyph_inverse_video_D();
    test_decode_glyph_flash_range_renders_normal();
    test_decode_glyph_null_safety();
    test_render_frame_renders_D_at_top_left();
    test_render_frame_row_interleave_second_row();
    test_render_frame_null_safety();
    printf("All tests passed.\n");
    return 0;
}
