#include "text_apple2.h"
#include "bio_display.h"
#include "lores_apple2.h" /* lores_byte_row_offsets[] -- shared text/lores row interleave */
#include "charrom_342_0133_a.h"

void text_apple2_decode_glyph(uint8_t screen_byte,
                               uint8_t out_bits[TEXT_APPLE2_CHAR_WIDTH_PX * TEXT_APPLE2_CHAR_HEIGHT_PX]) {
    if (!out_bits) {
        return; /* safe no-op on bad input */
    }

    /* Real classic Apple II text-mode display convention (verified
     * against MAME's apple2video.cpp source and confirmed by rendering
     * real captured DOS 3.3 boot-screen bytes -- see this module's
     * header comment and tests/test_text_apple2.c for the full
     * derivation):
     *   rom_index = screen_byte & 0x7F
     *   invert    = screen_byte < 0x40  (0x00-0x3F range: inverse video)
     *   0x40-0x7F range ("flash"): rendered normally here -- no ~2Hz
     *   flash timer implemented (documented simplification, matching
     *   this project's precedent for other un-timed peripherals). */
    uint8_t rom_index = (uint8_t)(screen_byte & 0x7F);
    int invert = (screen_byte < 0x40);

    for (int row = 0; row < TEXT_APPLE2_CHAR_HEIGHT_PX; row++) {
        uint8_t bits = charrom_342_0133_a[rom_index * TEXT_APPLE2_CHAR_HEIGHT_PX + row] & 0x7F;
        if (invert) {
            bits ^= 0x7F;
        }
        for (int col = 0; col < TEXT_APPLE2_CHAR_WIDTH_PX; col++) {
            /* Bit 0 = leftmost pixel, bit 6 = rightmost (fixed
             * 2026-08-02: the prior "bit 6 = leftmost" convention
             * produced mirror-reversed glyphs -- caught by Ryan
             * visually inspecting a real DOS 3.3 boot screenshot
             * zoomed in, confirmed reproducible via a standalone 'F'
             * glyph dump showing the vertical bar on the wrong side.
             * Verified against MAME's apple2video.cpp bit convention
             * directly this time, not just cited from memory. */
            out_bits[row * TEXT_APPLE2_CHAR_WIDTH_PX + col] =
                (uint8_t)((bits >> col) & 1);
        }
    }
}

void text_apple2_render_frame(int page2, read6502_fn read_mem, uint16_t *framebuffer) {
    if (!read_mem || !framebuffer) {
        return; /* safe no-op on bad input, matches every other renderer */
    }

    uint16_t base_addr = page2 ? TEXT_APPLE2_PAGE2_BASE_ADDR : TEXT_APPLE2_PAGE1_BASE_ADDR;
    uint16_t white = bio_display_color_to_rgb565(HIRES_COLOR_WHITE);
    uint16_t black = bio_display_color_to_rgb565(HIRES_COLOR_BLACK);

    for (int char_row = 0; char_row < TEXT_APPLE2_ROWS; char_row++) {
        /* SAME non-linear row-interleave table Lo-Res graphics uses --
         * text and Lo-Res share the exact same memory region/layout on
         * real Apple II hardware (see lores_apple2.h's own doc
         * comment), so this reuses lores_byte_row_offsets[] directly
         * rather than re-deriving an identical table. */
        uint16_t row_base = (uint16_t)(base_addr + lores_byte_row_offsets[char_row]);

        for (int char_col = 0; char_col < TEXT_APPLE2_COLS; char_col++) {
            uint8_t screen_byte = read_mem((uint16_t)(row_base + char_col));

            uint8_t glyph_bits[TEXT_APPLE2_CHAR_WIDTH_PX * TEXT_APPLE2_CHAR_HEIGHT_PX];
            text_apple2_decode_glyph(screen_byte, glyph_bits);

            int px_row_base = char_row * TEXT_APPLE2_CHAR_HEIGHT_PX;
            int px_col_base = char_col * TEXT_APPLE2_CHAR_WIDTH_PX;
            for (int dy = 0; dy < TEXT_APPLE2_CHAR_HEIGHT_PX; dy++) {
                int fb_row_base = (px_row_base + dy) * BIO_DISPLAY_WIDTH;
                for (int dx = 0; dx < TEXT_APPLE2_CHAR_WIDTH_PX; dx++) {
                    uint8_t bit = glyph_bits[dy * TEXT_APPLE2_CHAR_WIDTH_PX + dx];
                    framebuffer[fb_row_base + px_col_base + dx] = bit ? white : black;
                }
            }
        }
    }
}
