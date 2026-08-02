#ifndef TEXT_APPLE2_H
#define TEXT_APPLE2_H

#include <stdint.h>
#include "video_apple2.h" /* read6502_fn */

/*
 * BIO Core 0 domain: Apple II TEXT-mode character-ROM glyph rendering.
 *
 * Closes the gap documented in bio_display.h's own doc comment for
 * bio_display_render_frame_auto_text_aware(): until now, full TEXT mode
 * rendered solid black (no character-ROM renderer existed). This module
 * decodes real Apple II text-page screen memory ($0400-$07FF Page 1 /
 * $0800-$0BFF Page 2) through the real, MAME-verified 342-0133-a.chr
 * character generator ROM (see src/charrom_342_0133_a.h, extracted by
 * tools/gen_charrom_header.py -- NOT covered by this project's MIT
 * license, see LICENSE's exceptions list) into readable glyph pixels.
 *
 * Real Apple II text screen layout (verified against MAME's own
 * apple2video.cpp source, not assumed):
 *   - 40 columns x 24 rows of character cells, SAME base addresses and
 *     SAME non-linear row-interleave table as Lo-Res graphics (they
 *     share the same memory region) -- see lores_apple2.h's
 *     lores_byte_row_offsets[24], reused here directly rather than
 *     re-deriving an identical table.
 *   - Each character cell is 7 pixels wide x 8 pixels tall (scanlines).
 *     40*7=280, 24*8=192 -- an EXACT fit to BIO_DISPLAY_WIDTH x
 *     BIO_DISPLAY_HEIGHT, no scaling needed (same lucky fit as Lo-Res's
 *     40x48 blocks at 7x4px each).
 *   - Character ROM lookup: 342-0133-a.chr is 4096 bytes = 512 glyph
 *     definitions x 8 bytes/glyph (one byte per scanline row). Real
 *     classic Apple II (not IIe 80-column/alt-charset) display
 *     convention, confirmed by rendering known bytes from a REAL
 *     verified DOS 3.3 boot ("DOS VERSION 3.3") through this exact
 *     algorithm and visually confirming readable glyphs before writing
 *     any C code:
 *       - rom_index = screen_byte & 0x7F  (mask off the display-region
 *         high bit; only the low 7 bits select which of the ROM's first
 *         128 glyph slots to use -- classic II doesn't use the IIe's
 *         alt-charset/MouseText upper 256 glyphs)
 *       - invert = screen_byte < 0x40     (0x00-0x3F: true inverse video
 *         -- foreground/background swapped; 0x40-0x7F: "flash" range,
 *         rendered normally here since this module doesn't implement
 *         the ~2Hz hardware flash timer, matching this project's
 *         existing "documented simplification" precedent for other
 *         un-timed peripherals; 0x80-0xFF: normal, non-inverted)
 *       - bits = charrom_342_0133_a[rom_index * 8 + row] & 0x7F, then
 *         XOR 0x7F if invert -- 7 significant bits per row, MSB-first
 *         (bit 6 = leftmost pixel, bit 0 = rightmost)
 *
 * Colors: renders as bio_display's WHITE (lit) / BLACK (unlit) --
 * matches the monochrome-glyph nature of real Apple II text mode
 * (no per-character color in classic/II+ text mode); CRT mode
 * (color/green-phosphor/amber) is applied via
 * bio_display_color_to_rgb565() exactly like every other renderer in
 * this codebase, not reinvented here.
 */

#define TEXT_APPLE2_PAGE1_BASE_ADDR 0x0400u
#define TEXT_APPLE2_PAGE2_BASE_ADDR 0x0800u
#define TEXT_APPLE2_COLS            40
#define TEXT_APPLE2_ROWS            24
#define TEXT_APPLE2_CHAR_WIDTH_PX   7
#define TEXT_APPLE2_CHAR_HEIGHT_PX  8

/*
 * Render one character cell's glyph into a caller-provided 7x8 buffer of
 * hires_color_t-style 0/1 values (1 = lit/foreground, 0 = unlit/
 * background) -- NOT RGB565 directly, so this can be unit-tested against
 * exact expected bit patterns without depending on any particular RGB565
 * palette mapping. out_bits must have room for
 * TEXT_APPLE2_CHAR_WIDTH_PX * TEXT_APPLE2_CHAR_HEIGHT_PX (56) entries,
 * row-major (row * TEXT_APPLE2_CHAR_WIDTH_PX + col).
 *
 * screen_byte is the raw byte read from Apple II text-page memory (NOT
 * pre-masked by the caller -- this function does the & 0x7F / inverse
 * logic itself, matching real hardware's own interpretation of the
 * high bits).
 *
 * Safety: NULL out_bits is a silent no-op.
 */
void text_apple2_decode_glyph(uint8_t screen_byte,
                               uint8_t out_bits[TEXT_APPLE2_CHAR_WIDTH_PX * TEXT_APPLE2_CHAR_HEIGHT_PX]);

/*
 * Render the full 40x24 text screen into a BIO_DISPLAY_WIDTH x
 * BIO_DISPLAY_HEIGHT (280x192) RGB565 framebuffer -- an exact pixel fit,
 * no scaling. page2 matches apple2_mem_is_page2_selected()'s convention
 * (0 = Page 1 $0400, nonzero = Page 2 $0800).
 *
 * Safety: NULL read_mem or framebuffer is a silent no-op, matching every
 * other renderer in this codebase (video_apple2.c, lores_apple2.c,
 * bio_display.c).
 */
void text_apple2_render_frame(int page2, read6502_fn read_mem,
                               uint16_t *framebuffer);

#endif /* TEXT_APPLE2_H */
