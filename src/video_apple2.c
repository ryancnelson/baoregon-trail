#include "video_apple2.h"

/*
 * RED phase stub: table not yet populated (only row 0 filled in, rest 0 as
 * a deliberate placeholder), and the decode function is intentionally
 * unimplemented. This must fail the test in tests/test_video_apple2.c for
 * the right reason (output pixels never get written -> stay poisoned),
 * not from a build/link error, before we write the real logic.
 */
const uint16_t hires_line_offsets[HIRES_ROWS] = {
    0x0000, 0x0400, 0x0800, 0x0c00, 0x1000, 0x1400, 0x1800, 0x1c00,
    0x0080, 0x0480, 0x0880, 0x0c80, 0x1080, 0x1480, 0x1880, 0x1c80,
    /* remaining rows populated as needed */
};

void hires_decode_scanline_mono(int row, read6502_fn read_mem,
                                 uint8_t out_pixels[HIRES_PIXELS_WIDE]) {
    uint16_t row_base = HIRES_BASE_ADDR + hires_line_offsets[row];

    for (int byte_idx = 0; byte_idx < HIRES_COLS_BYTES; byte_idx++) {
        uint8_t byte = read_mem(row_base + byte_idx);
        int pixel_base = byte_idx * 7;
        for (int bit = 0; bit < 7; bit++) {
            out_pixels[pixel_base + bit] = (byte >> bit) & 0x01;
        }
    }
}
