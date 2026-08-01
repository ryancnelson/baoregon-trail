#ifndef LORES_APPLE2_H
#define LORES_APPLE2_H

#include <stdint.h>
#include "video_apple2.h"

/*
 * BIO Core 0 domain: Apple II Lo-Res (40x48 block) graphics decoding.
 *
 * Lo-Res graphics live in the TEXT PAGE memory range ($0400-$07FF for
 * Page 1, $0800-$0BFF for Page 2) -- NOT the Hi-Res $2000-$3FFF region.
 * Selected via apple2_mem_is_hires_mode() returning 0 (LORES, $C056) with
 * apple2_mem_is_text_mode() returning 0 (GRAPHICS, $C050).
 *
 * Real Apple II Lo-Res format:
 *   - 40 columns x 24 byte-rows in memory, but each byte encodes TWO
 *     vertically-stacked color blocks (top block in bits 0-3, bottom
 *     block in bits 4-7) -- so the visible resolution is 40x48 blocks.
 *   - 16 possible colors per nibble (standard Apple II Lo-Res palette).
 *   - The 24 byte-rows use the SAME structural interleave pattern as
 *     Hi-Res's 192-row table, just with a shorter period (row%8 * 0x80 +
 *     row/8 * 0x28) since there are 3x fewer rows and a byte covers 2
 *     visible rows instead of 7 pixels.
 *
 * This module intentionally does NOT touch RGB565/hires_color_t --
 * Lo-Res's 16-color palette is a distinct color space from Hi-Res's
 * 6-color NTSC-artifact palette. A shared RGB565 mapping is a follow-up
 * iteration (see bio_display.c for where the Hi-Res equivalent lives).
 */

#define LORES_PAGE1_BASE_ADDR 0x0400u
#define LORES_PAGE2_BASE_ADDR 0x0800u
#define LORES_BYTE_ROWS        24    /* 24 bytes tall in memory */
#define LORES_COLS_BYTES       40    /* 40 bytes per byte-row */
#define LORES_BLOCK_ROWS        48   /* 2 visible blocks per byte -> 48 rows */
#define LORES_BLOCK_COLS        40   /* 1 block per byte horizontally */

/* Precomputed 24-entry Lo-Res byte-row -> memory offset lookup table.
 * Structurally the same interleave family as hires_line_offsets[], just
 * a 3x shorter period: offset(row) = (row%8)*0x80 + (row/8)*0x28.
 */
extern const uint16_t lores_byte_row_offsets[LORES_BYTE_ROWS];

/*
 * Decode the full Lo-Res screen into a 40x48 grid of 4-bit color indices
 * (0-15, standard Apple II Lo-Res palette order -- see BRAINSTORM.md for
 * the RGB mapping once display calibration happens). out_blocks must have
 * room for LORES_BLOCK_COLS * LORES_BLOCK_ROWS (40*48=1920) entries,
 * row-major (block_row * LORES_BLOCK_COLS + block_col).
 *
 * page2 is 0 for Page 1 ($0400) or nonzero for Page 2 ($0800), matching
 * apple2_mem_is_page2_selected()'s return convention.
 */
void lores_decode_screen_page(int page2, read6502_fn read_mem,
                               uint8_t out_blocks[LORES_BLOCK_COLS * LORES_BLOCK_ROWS]);

#endif /* LORES_APPLE2_H */
