#ifndef VIDEO_APPLE2_H
#define VIDEO_APPLE2_H

#include <stdint.h>

/*
 * BIO Core 0 domain: Apple II Hi-Res (280x192) video un-swizzling.
 *
 * Contract with Woz's cpu6502.c (locked 2026-07-31):
 *   uint8_t read6502(uint16_t address);
 *   void    write6502(uint16_t address, uint8_t value);
 *
 * apple2_mem.c (Step 3, ours+Duke's) backs $2000-$3FFF with the raw Apple II
 * Hi-Res buffer in SRAM behind those two functions. Until apple2_mem.c/
 * cpu6502.c land, tests inject a mock matching this same signature so the
 * un-swizzle logic and the real memory backing can be developed in parallel
 * and swapped later without touching this file (per Ryan's steer 2026-07-31:
 * don't block on Woz landing, mock now, swap later, DEF CON deadline is real).
 */
typedef uint8_t (*read6502_fn)(uint16_t address);

#define HIRES_BASE_ADDR   0x2000u
#define HIRES_COLS_BYTES  40      /* 40 bytes per scanline = 280 pixels / 7 */
#define HIRES_ROWS        192
#define HIRES_PIXELS_WIDE 280

/* Precomputed 192-entry Hi-Res row->byte-offset lookup table.
 * See BRAINSTORM.md section 2 for the derivation (interleaved 8/8/8 banding).
 */
extern const uint16_t hires_line_offsets[HIRES_ROWS];

/*
 * Decode one Hi-Res scanline into a monochrome pixel buffer.
 *
 * out_pixels must have room for HIRES_PIXELS_WIDE (280) bytes; each byte is
 * 0 (off) or 1 (on). Color/artifacting rules are NOT applied here -- see
 * hires_decode_scanline_color() below. This first pass only proves the
 * row-offset table and the 7-bit-group -> pixel expansion are correct.
 *
 * row must be in [0, HIRES_ROWS).
 */
void hires_decode_scanline_mono(int row, read6502_fn read_mem,
                                 uint8_t out_pixels[HIRES_PIXELS_WIDE]);

/*
 * Apple II Hi-Res NTSC artifact colors (BRAINSTORM.md section 2 step 4).
 *
 * Classic Apple II Hi-Res composite-video artifacting model: an isolated
 * lit pixel (no lit neighbor immediately before or after it) renders as one
 * of 4 colors based on (a) its column parity (even/odd) and (b) the
 * high bit (bit 7, the "palette bit") of the byte it came from. Two or more
 * *consecutive* lit pixels merge into WHITE (the color subcarrier phases
 * cancel). An unlit pixel with no lit neighbors is BLACK.
 *
 *   high bit = 0 (palette group "violet/green"): even col -> GREEN,  odd col -> VIOLET
 *   high bit = 1 (palette group "blue/orange"):  even col -> ORANGE, odd col -> BLUE
 */
typedef enum {
    HIRES_COLOR_BLACK = 0,
    HIRES_COLOR_GREEN,
    HIRES_COLOR_VIOLET,
    HIRES_COLOR_ORANGE,
    HIRES_COLOR_BLUE,
    HIRES_COLOR_WHITE,
} hires_color_t;

/*
 * Decode one Hi-Res scanline into NTSC-artifact colors.
 *
 * out_colors must have room for HIRES_PIXELS_WIDE (280) entries. Internally
 * calls hires_decode_scanline_mono() for the raw on/off bits plus tracks the
 * per-byte high bit, then applies the artifacting rule above per pixel.
 *
 * row must be in [0, HIRES_ROWS).
 */
void hires_decode_scanline_color(int row, read6502_fn read_mem,
                                  hires_color_t out_colors[HIRES_PIXELS_WIDE]);

#endif /* VIDEO_APPLE2_H */
