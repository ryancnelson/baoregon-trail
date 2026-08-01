#include "bio_display.h"

/*
 * RGB565 palette for the 6 Apple II Hi-Res artifact colors. Approximate
 * values (not yet calibrated against a real NTSC decoder or physical
 * badge display -- that calibration pass is future work once real
 * hardware exists, per MEMORY.md 2026-07-31's host-simulator-first note).
 *
 * RGB565 layout: RRRRRGGGGGGBBBBB (5-6-5 bits).
 */
static const uint16_t g_color_to_rgb565[] = {
    [HIRES_COLOR_BLACK]  = 0x0000, /* 0,0,0 */
    [HIRES_COLOR_GREEN]  = 0x07E0, /* 0,255,0 -- pure green (max 6-bit G) */
    [HIRES_COLOR_VIOLET] = 0x781F, /* violet approximation: high R, low G, max B */
    [HIRES_COLOR_ORANGE] = 0xFC00, /* orange approximation: max R, mid G, 0 B */
    [HIRES_COLOR_BLUE]   = 0x001F, /* pure blue (max 5-bit B) */
    [HIRES_COLOR_WHITE]  = 0xFFFF, /* 255,255,255 */
};

uint16_t bio_display_color_to_rgb565(hires_color_t color) {
    return g_color_to_rgb565[color];
}

void bio_display_render_frame(read6502_fn read_mem,
                               uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]) {
    for (int row = 0; row < BIO_DISPLAY_HEIGHT; row++) {
        hires_color_t row_colors[BIO_DISPLAY_WIDTH];
        hires_decode_scanline_color(row, read_mem, row_colors);

        int row_base = row * BIO_DISPLAY_WIDTH;
        for (int col = 0; col < BIO_DISPLAY_WIDTH; col++) {
            framebuffer[row_base + col] = bio_display_color_to_rgb565(row_colors[col]);
        }
    }
}

/* DMA push stub state -- see bio_display.h doc comment: no real SPI/DMA
 * peripheral exists yet, this just records the call for host-test
 * inspection until baochip's hardware-init sequence / register map lands. */
static const uint16_t *g_last_dma_framebuffer = 0;
static uint32_t g_last_dma_pixel_count = 0;
static int g_dma_push_was_called = 0;

void bio_display_dma_push(const uint16_t *framebuffer, uint32_t pixel_count) {
    g_last_dma_framebuffer = framebuffer;
    g_last_dma_pixel_count = pixel_count;
    g_dma_push_was_called = 1;
}

int bio_display_last_dma_push(const uint16_t **out_framebuffer, uint32_t *out_pixel_count) {
    if (!g_dma_push_was_called) {
        return 0;
    }
    *out_framebuffer = g_last_dma_framebuffer;
    *out_pixel_count = g_last_dma_pixel_count;
    return 1;
}
