#include "lores_apple2.h"

/*
 * 24-entry Lo-Res byte-row -> memory offset lookup table. Same structural
 * interleave family as hires_line_offsets[] (see video_apple2.c), just a
 * 3x shorter period since Lo-Res's text page has 1/8th the byte rows of
 * Hi-Res's bitmap and no third-level (row/64) band:
 *   offset(row) = (row % 8) * 0x80 + (row / 8) * 0x28
 */
const uint16_t lores_byte_row_offsets[LORES_BYTE_ROWS] = {
    0x0000, 0x0080, 0x0100, 0x0180, 0x0200, 0x0280, 0x0300, 0x0380,
    0x0028, 0x00A8, 0x0128, 0x01A8, 0x0228, 0x02A8, 0x0328, 0x03A8,
    0x0050, 0x00D0, 0x0150, 0x01D0, 0x0250, 0x02D0, 0x0350, 0x03D0,
};

void lores_decode_screen_page(int page2, read6502_fn read_mem,
                               uint8_t out_blocks[LORES_BLOCK_COLS * LORES_BLOCK_ROWS]) {
    uint16_t base_addr = page2 ? LORES_PAGE2_BASE_ADDR : LORES_PAGE1_BASE_ADDR;

    for (int byte_row = 0; byte_row < LORES_BYTE_ROWS; byte_row++) {
        uint16_t row_base = base_addr + lores_byte_row_offsets[byte_row];

        for (int col = 0; col < LORES_COLS_BYTES; col++) {
            uint8_t byte = read_mem(row_base + col);
            uint8_t top_color = byte & 0x0F;
            uint8_t bottom_color = (byte >> 4) & 0x0F;

            int top_block_row = byte_row * 2;
            int bottom_block_row = top_block_row + 1;

            out_blocks[top_block_row * LORES_BLOCK_COLS + col] = top_color;
            out_blocks[bottom_block_row * LORES_BLOCK_COLS + col] = bottom_color;
        }
    }
}

uint16_t lores_color_to_rgb565(uint8_t color) {
    /* Standard Apple II Lo-Res 16-color palette, RGB565 approximation
     * (5-6-5 bits). Same "not yet calibrated against real hardware"
     * caveat as bio_display_color_to_rgb565() -- see MEMORY.md
     * 2026-07-31's host-simulator-first note. Palette order and names
     * per the standard, widely-documented Apple II Lo-Res color table. */
    static const uint16_t palette[16] = {
        0x0000, /*  0 Black       */
        0x9000, /*  1 Deep Red    */
        0x000D, /*  2 Dark Blue   */
        0xA0B8, /*  3 Purple      */
        0x0320, /*  4 Dark Green  */
        0x738E, /*  5 Gray 1      */
        0x055F, /*  6 Medium Blue */
        0x7BFF, /*  7 Light Blue  */
        0x5300, /*  8 Brown       */
        0xFC60, /*  9 Orange      */
        0xC618, /* 10 Gray 2      */
        0xFB56, /* 11 Pink        */
        0x07E0, /* 12 Green       */
        0xFFA0, /* 13 Yellow      */
        0x7FF5, /* 14 Aqua        */
        0xFFFF, /* 15 White       */
    };

    if (color >= 16) {
        return 0x0000; /* out-of-range: black fallback */
    }
    return palette[color];
}
