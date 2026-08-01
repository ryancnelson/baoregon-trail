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
#define HIRES_PAGE2_BASE_ADDR 0x4000u /* real Apple II Hi-Res Page 2, $4000-$5FFF */
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
 * row must be in [0, HIRES_ROWS). Always targets Hi-Res PAGE 1 ($2000) --
 * see hires_decode_scanline_mono_page() for page-2-aware decoding, driven
 * by apple2_mem_is_page2_selected() ($C054/$C055 soft-switch).
 *
 * Safety (matches the disk_trap.c / bunnie_audio.c convention of
 * "safe no-op on bad input, never crash"): an out-of-range row or a NULL
 * read_mem/out_pixels is a silent no-op -- out_pixels is left untouched
 * (not even zeroed) rather than reading hires_line_offsets[] out of
 * bounds or dereferencing a NULL function pointer.
 */
void hires_decode_scanline_mono(int row, read6502_fn read_mem,
                                 uint8_t out_pixels[HIRES_PIXELS_WIDE]);

/*
 * Page-aware variant of hires_decode_scanline_mono(): page2 is 0 for
 * Page 1 ($2000-$3FFF) or nonzero for Page 2 ($4000-$5FFF), matching
 * apple2_mem_is_page2_selected()'s return convention exactly so callers
 * can pass that value straight through. hires_decode_scanline_mono(row,
 * read_mem, out) is equivalent to
 * hires_decode_scanline_mono_page(row, 0, read_mem, out).
 *
 * Same safety contract as hires_decode_scanline_mono(): out-of-range row
 * or NULL read_mem/out_pixels is a silent no-op.
 */
void hires_decode_scanline_mono_page(int row, int page2, read6502_fn read_mem,
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
 * row must be in [0, HIRES_ROWS). Always targets Hi-Res PAGE 1 ($2000) --
 * see hires_decode_scanline_color_page() for page-2-aware decoding.
 *
 * Safety: out-of-range row or NULL read_mem/out_colors is a silent
 * no-op, same convention as hires_decode_scanline_mono().
 */
void hires_decode_scanline_color(int row, read6502_fn read_mem,
                                  hires_color_t out_colors[HIRES_PIXELS_WIDE]);

/*
 * Page-aware variant of hires_decode_scanline_color(): page2 is 0 for
 * Page 1 ($2000-$3FFF) or nonzero for Page 2 ($4000-$5FFF), matching
 * apple2_mem_is_page2_selected()'s return convention. Both the on/off
 * bits AND the palette (high) bit are read from the selected page.
 * hires_decode_scanline_color(row, read_mem, out) is equivalent to
 * hires_decode_scanline_color_page(row, 0, read_mem, out).
 *
 * Safety: out-of-range row or NULL read_mem/out_colors is a silent
 * no-op.
 */
void hires_decode_scanline_color_page(int row, int page2, read6502_fn read_mem,
                                       hires_color_t out_colors[HIRES_PIXELS_WIDE]);

#endif /* VIDEO_APPLE2_H */
