#ifndef BIO_DISPLAY_H
#define BIO_DISPLAY_H

#include <stdint.h>
#include "video_apple2.h"
#include "lores_apple2.h"

/*
 * BIO Core 0 domain: display DMA driver stub.
 *
 * Per BRAINSTORM.md section 2 step 5 and CLAUDE.md's mission ("Scale 280x192
 * to target badge display resolution ... push via SPI DMA"), this module is
 * the layer ABOVE hires_decode_scanline_color(): it drives a full-frame
 * decode into an RGB565 framebuffer and stages it for the (not-yet-existing,
 * pre-silicon) SPI DMA peripheral.
 *
 * Scope of this iteration (a DRIVER STUB, not final hardware code):
 *   1. hires_color_t -> RGB565 palette lookup (real Apple II NTSC-ish
 *      approximations -- exact values are a later color-calibration pass
 *      once real hardware/display is available to eyeball against).
 *   2. bio_display_render_frame(): decodes all 192 rows via
 *      hires_decode_scanline_color() and writes RGB565 pixels into a
 *      caller-provided framebuffer at native 280x192 resolution. Scaling to
 *      320x240/480x320 is explicitly NOT done here -- that's a follow-up
 *      iteration once a target display resolution is confirmed by baochip.
 *   3. bio_display_dma_push(): STUB ONLY. No real SPI/DMA peripheral exists
 *      yet (Baochip-1x silicon isn't in hand -- see MEMORY.md 2026-07-31).
 *      This function's host-testable contract is just "was called with the
 *      right framebuffer pointer/size" -- real register pokes land once
 *      baochip's hardware-init sequence / SPI DMA register map exists.
 *
 * Register-level BIO Core 0 code (RV32E asm, actual SPI DMA MMIO writes)
 * is explicitly future work -- not in scope until the hardware-init
 * sequence and SPI peripheral register map are confirmed (this is a
 * cross-domain dependency on baochip, not something to invent solo).
 */

#define BIO_DISPLAY_WIDTH  HIRES_PIXELS_WIDE  /* 280, native, no scaling yet */
#define BIO_DISPLAY_HEIGHT HIRES_ROWS         /* 192, native, no scaling yet */

/* CRT monochrome/color display modes (P31 Green Phosphor, Amber CRT, or NTSC Color) */
typedef enum {
    BIO_CRT_MODE_COLOR = 0,
    BIO_CRT_MODE_GREEN_PHOSPHOR = 1,
    BIO_CRT_MODE_AMBER = 2
} bio_crt_mode_t;

void bio_display_set_crt_mode(bio_crt_mode_t mode);
bio_crt_mode_t bio_display_get_crt_mode(void);

/* Convert one hires_color_t to its RGB565 (5-6-5) approximation.
 * Safety: an out-of-range color value returns 0x0000 (black fallback)
 * instead of an out-of-bounds array read, matching
 * lores_color_to_rgb565()'s convention. */
uint16_t bio_display_color_to_rgb565(hires_color_t color);

/*
 * Decode all HIRES_ROWS scanlines via hires_decode_scanline_color() and
 * write RGB565 pixels into framebuffer (row-major, BIO_DISPLAY_WIDTH *
 * BIO_DISPLAY_HEIGHT entries, caller-allocated). Always renders Hi-Res
 * PAGE 1 -- see bio_display_render_frame_page() for page-2-aware
 * rendering, driven by apple2_mem_is_page2_selected().
 *
 * Safety: NULL read_mem or framebuffer is a silent no-op (matches
 * video_apple2.c / lores_apple2.c's convention -- never crash on bad
 * input).
 */
void bio_display_render_frame(read6502_fn read_mem,
                               uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]);

/*
 * Page-aware variant of bio_display_render_frame(): page2 is 0 for Page 1
 * ($2000-$3FFF) or nonzero for Page 2 ($4000-$5FFF), matching
 * apple2_mem_is_page2_selected()'s return convention exactly so callers
 * (e.g. the eventual BIO Core 0 render loop) can pass that value straight
 * through. bio_display_render_frame(read_mem, fb) is equivalent to
 * bio_display_render_frame_page(0, read_mem, fb).
 *
 * Safety: NULL read_mem or framebuffer is a silent no-op.
 */
void bio_display_render_frame_page(int page2, read6502_fn read_mem,
                                    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]);

/*
 * Real Apple II MIXED mode (driven by apple2_mem_is_mixed_mode(),
 * $C052/$C053) always shows the bottom 4 text rows (32 scanlines) as TEXT
 * regardless of the HIRES/LORES soft-switch -- only the top 160 scanlines
 * (BIO_DISPLAY_HEIGHT - HIRES_MIXED_MODE_TEXT_ROWS) show graphics. Text
 * character rendering itself (character ROM lookup, 40-column layout) is
 * OUT OF SCOPE here -- that's a different subsystem (no character ROM
 * data exists in this codebase yet). This module's job is only to know
 * WHERE graphics rendering must stop so it doesn't overwrite/conflict
 * with whatever renders the text region.
 */
#define HIRES_MIXED_MODE_TEXT_ROWS 32 /* bottom 4 text rows * 8 px/row */
#define HIRES_MIXED_MODE_GRAPHICS_ROWS (BIO_DISPLAY_HEIGHT - HIRES_MIXED_MODE_TEXT_ROWS) /* 160 */

/*
 * Mixed-mode-aware variant of bio_display_render_frame_page(): when
 * mixed_mode is nonzero, only rows [0, HIRES_MIXED_MODE_GRAPHICS_ROWS)
 * are decoded into framebuffer -- rows
 * [HIRES_MIXED_MODE_GRAPHICS_ROWS, BIO_DISPLAY_HEIGHT) are left
 * UNTOUCHED (not zeroed, not black -- the caller/text-mode renderer owns
 * that region and must fill it separately). When mixed_mode is 0, all
 * BIO_DISPLAY_HEIGHT rows are decoded exactly as
 * bio_display_render_frame_page() already does.
 */
void bio_display_render_frame_mixed(int page2, int mixed_mode, read6502_fn read_mem,
                                     uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]);

/*
 * Decode the full Lo-Res screen (via lores_decode_screen_page(), see
 * lores_apple2.h) into the SAME native 280x192 RGB565 framebuffer shape
 * as bio_display_render_frame*() use. Each Lo-Res block is exactly 7x4
 * pixels in the 280x192 space (280/40=7, 192/48=4 -- an exact fit, no
 * fractional scaling needed), so every pixel within a block's footprint
 * gets that block's color.
 *
 * page2 matches apple2_mem_is_page2_selected()'s convention (Lo-Res Page
 * 1 is $0400, Page 2 is $0800 -- see lores_apple2.h).
 */
void bio_display_render_lores_frame(int page2, read6502_fn read_mem,
                                     uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]);

/*
 * Mixed-mode-aware variant of bio_display_render_lores_frame(): real
 * Apple II MIXED mode ($C052/$C053) forces the bottom 4 text rows (32
 * scanlines) to TEXT regardless of HIRES/LORES, exactly like
 * bio_display_render_frame_mixed()'s HIRES path. In Lo-Res's 48-block-row
 * grid (4px/block), that's the bottom 8 block rows
 * (HIRES_MIXED_MODE_TEXT_ROWS / 4 = 8) -- LORES_BLOCK_ROWS - 8 = 40
 * block rows of graphics remain. When mixed_mode is nonzero, pixel rows
 * [HIRES_MIXED_MODE_GRAPHICS_ROWS, BIO_DISPLAY_HEIGHT) are left
 * UNTOUCHED (same convention as bio_display_render_frame_mixed()) --
 * owned by a future text-mode renderer, not this module.
 */
void bio_display_render_lores_frame_mixed(int page2, int mixed_mode, read6502_fn read_mem,
                                           uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]);

/*
 * Mode-aware entry point: picks Hi-Res or Lo-Res rendering based on the
 * hires_mode flag, matching apple2_mem_is_hires_mode()'s convention
 * (nonzero = HIRES/$C057, 0 = LORES/$C056). This is the closest thing to
 * a single "render whatever mode is currently active" call this module
 * offers -- TEXT mode ($C050/$C051, character ROM rendering) is
 * explicitly NOT handled here, since no character ROM data exists in
 * this codebase yet (see bio_display_render_frame_mixed()'s doc comment
 * for the same caveat on the MIXED-mode text region). Callers in TEXT
 * mode should not call this function -- see
 * bio_display_render_frame_auto_text_aware() for a variant that DOES
 * check apple2_mem_is_text_mode() and is safe to call unconditionally.
 *
 * mixed_mode is honored for BOTH the HIRES path (via
 * bio_display_render_frame_mixed()) and the LORES path (via
 * bio_display_render_lores_frame_mixed()) -- both leave the bottom 4
 * text rows (32 scanlines) untouched when mixed_mode is set.
 */
void bio_display_render_frame_auto(int hires_mode, int page2, int mixed_mode,
                                    read6502_fn read_mem,
                                    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]);

/*
 * Text-mode-aware variant of bio_display_render_frame_auto(): a REAL
 * bug fix, not just a new feature. Real Apple II hardware's TEXT/
 * GRAPHICS soft-switch ($C050/$C051) is INDEPENDENT of HIRES/LORES, and
 * the machine defaults to TEXT mode post-reset (NOT graphics) -- but
 * bio_display_render_frame_auto() never checked it, so it always
 * rendered Hi-Res/Lo-Res graphics garbage into the framebuffer even in
 * full TEXT mode (e.g. the boot splash screen, or any TEXT-mode 6502
 * program, would show block-graphics noise instead of a blank/text
 * screen).
 *
 * text_mode matches apple2_mem_is_text_mode()'s convention (nonzero =
 * TEXT/$C051, 0 = GRAPHICS/$C050). When text_mode is nonzero AND
 * mixed_mode is 0 (full-screen TEXT, not MIXED), this function fills
 * framebuffer with BLACK (0x0000), NOT left-untouched -- unlike MIXED
 * mode's text-region convention (where a future text-mode renderer is
 * expected to run every frame and fill that region itself), there is
 * currently no text-mode renderer AT ALL in this codebase, so
 * "untouched" would mean "whatever the previous frame's Hi-Res/Lo-Res
 * graphics happened to leave there" -- genuinely stale content, not a
 * safe placeholder. Filling black avoids that regression until a real
 * character-ROM-backed text renderer exists (no character ROM data
 * exists in this codebase yet -- that renderer is explicitly OUT OF
 * SCOPE here). When mixed_mode is nonzero, TEXT/GRAPHICS is irrelevant
 * (real Apple II MIXED mode always shows graphics on top regardless of
 * the TEXT/GRAPHICS switch) -- this function proceeds with the normal
 * HIRES/LORES + MIXED-boundary dispatch (bio_display_render_frame_auto())
 * in that case, which DOES still leave its bottom text rows untouched
 * (a future MIXED-mode text renderer is expected to run every frame,
 * unlike the full-TEXT case where nothing runs at all).
 */
void bio_display_render_frame_auto_text_aware(int hires_mode, int page2, int mixed_mode,
                                               int text_mode, read6502_fn read_mem,
                                               uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]);

/*
 * DMA push stub. Records that a push was requested (pointer + size) via
 * bio_display_last_dma_push() for host-test inspection; performs no real
 * hardware I/O (there is no SPI/DMA peripheral to talk to yet -- see
 * module doc comment above).
 *
 * Architecture update (BRAINSTORM.md section 6B/6C, bunnie huang
 * confirmed): the real hardware path will NOT use the PL230 MDMA
 * controller -- it's a confirmed dead end on physical Baochip-1x
 * silicon (config-register writes silently fail). Real display refresh
 * will instead hand off to BIO Core 3 (a dedicated 700 MHz memcopy DMA
 * engine, per the BIO Coprocessor Cluster Map), not a direct SPI/DMA
 * write from BIO Core 0 itself. Still deferred pending baochip's actual
 * BIO Core 3 register map/hardware-init sequence -- cross-domain
 * dependency, not something to invent solo.
 */
void bio_display_dma_push(const uint16_t *framebuffer, uint32_t pixel_count);

/* Test/inspection hook: what was the most recent bio_display_dma_push()
 * call's arguments? Returns 0 (false) if never called. */
int bio_display_last_dma_push(const uint16_t **out_framebuffer, uint32_t *out_pixel_count);

#endif /* BIO_DISPLAY_H */
