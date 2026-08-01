#ifndef BIO_DISPLAY_H
#define BIO_DISPLAY_H

#include <stdint.h>
#include "video_apple2.h"

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

/* Convert one hires_color_t to its RGB565 (5-6-5) approximation. */
uint16_t bio_display_color_to_rgb565(hires_color_t color);

/*
 * Decode all HIRES_ROWS scanlines via hires_decode_scanline_color() and
 * write RGB565 pixels into framebuffer (row-major, BIO_DISPLAY_WIDTH *
 * BIO_DISPLAY_HEIGHT entries, caller-allocated). Always renders Hi-Res
 * PAGE 1 -- see bio_display_render_frame_page() for page-2-aware
 * rendering, driven by apple2_mem_is_page2_selected().
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
 */
void bio_display_render_frame_page(int page2, read6502_fn read_mem,
                                    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]);

/*
 * DMA push stub. Records that a push was requested (pointer + size) via
 * bio_display_last_dma_push() for host-test inspection; performs no real
 * hardware I/O (there is no SPI/DMA peripheral to talk to yet -- see
 * module doc comment above).
 */
void bio_display_dma_push(const uint16_t *framebuffer, uint32_t pixel_count);

/* Test/inspection hook: what was the most recent bio_display_dma_push()
 * call's arguments? Returns 0 (false) if never called. */
int bio_display_last_dma_push(const uint16_t **out_framebuffer, uint32_t *out_pixel_count);

#endif /* BIO_DISPLAY_H */
