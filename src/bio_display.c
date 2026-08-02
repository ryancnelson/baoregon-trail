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

static bio_crt_mode_t g_crt_mode = BIO_CRT_MODE_COLOR;

void bio_display_set_crt_mode(bio_crt_mode_t mode) {
    if (mode >= BIO_CRT_MODE_COLOR && mode <= BIO_CRT_MODE_AMBER) {
        g_crt_mode = mode;
    }
}

bio_crt_mode_t bio_display_get_crt_mode(void) {
    return g_crt_mode;
}

uint16_t bio_display_color_to_rgb565(hires_color_t color) {
    if (color < HIRES_COLOR_BLACK || color > HIRES_COLOR_WHITE) {
        return 0x0000; /* out-of-range: black fallback, matches lores_color_to_rgb565() */
    }
    uint16_t rgb = g_color_to_rgb565[color];
    if (g_crt_mode == BIO_CRT_MODE_COLOR) {
        return rgb;
    }

    /* Extract 8-bit R, G, B */
    uint8_t r8 = (uint8_t)(((rgb >> 11) & 0x1F) * 255 / 31);
    uint8_t g8 = (uint8_t)(((rgb >> 5) & 0x3F) * 255 / 63);
    uint8_t b8 = (uint8_t)((rgb & 0x1F) * 255 / 31);

    /* Luminance Y = 0.299*R + 0.587*G + 0.114*B */
    uint32_t y = (uint32_t)(r8 * 77 + g8 * 150 + b8 * 29) >> 8;

    if (g_crt_mode == BIO_CRT_MODE_GREEN_PHOSPHOR) {
        /* Green Phosphor (P31): Luminance Y mapped to 6-bit Green channel */
        uint16_t g6 = (uint16_t)((y * 63 + 127) / 255);
        return (g6 << 5);
    } else if (g_crt_mode == BIO_CRT_MODE_AMBER) {
        /* Amber CRT (#FFB000): R max, G = 176/255 of R, B = 0 */
        uint16_t r5 = (uint16_t)((y * 31 + 127) / 255);
        uint16_t g6 = (uint16_t)((y * 176 * 63 / 255 + 127) / 255);
        return (r5 << 11) | (g6 << 5);
    }
    return rgb;
}

void bio_display_render_frame(read6502_fn read_mem,
                               uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]) {
    bio_display_render_frame_page(0, read_mem, framebuffer);
}

void bio_display_render_frame_page(int page2, read6502_fn read_mem,
                                    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]) {
    if (!read_mem || !framebuffer) {
        return; /* safe no-op on bad input */
    }

    for (int row = 0; row < BIO_DISPLAY_HEIGHT; row++) {
        hires_color_t row_colors[BIO_DISPLAY_WIDTH];
        hires_decode_scanline_color_page(row, page2, read_mem, row_colors);

        int row_base = row * BIO_DISPLAY_WIDTH;
        for (int col = 0; col < BIO_DISPLAY_WIDTH; col++) {
            framebuffer[row_base + col] = bio_display_color_to_rgb565(row_colors[col]);
        }
    }
}

void bio_display_render_frame_mixed(int page2, int mixed_mode, read6502_fn read_mem,
                                     uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]) {
    if (!read_mem || !framebuffer) {
        return;
    }

    int rows_to_render = mixed_mode ? HIRES_MIXED_MODE_GRAPHICS_ROWS : BIO_DISPLAY_HEIGHT;

    for (int row = 0; row < rows_to_render; row++) {
        hires_color_t row_colors[BIO_DISPLAY_WIDTH];
        hires_decode_scanline_color_page(row, page2, read_mem, row_colors);

        int row_base = row * BIO_DISPLAY_WIDTH;
        for (int col = 0; col < BIO_DISPLAY_WIDTH; col++) {
            framebuffer[row_base + col] = bio_display_color_to_rgb565(row_colors[col]);
        }
    }
    /* Rows [rows_to_render, BIO_DISPLAY_HEIGHT) intentionally left
     * untouched when mixed_mode is set -- owned by a future text-mode
     * renderer, not this module (see bio_display.h doc comment). */
}

void bio_display_render_lores_frame(int page2, read6502_fn read_mem,
                                     uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]) {
    if (!read_mem || !framebuffer) {
        return;
    }

    uint8_t blocks[LORES_BLOCK_COLS * LORES_BLOCK_ROWS];
    lores_decode_screen_page(page2, read_mem, blocks);

    for (int block_row = 0; block_row < LORES_BLOCK_ROWS; block_row++) {
        for (int block_col = 0; block_col < LORES_BLOCK_COLS; block_col++) {
            uint8_t color_index = blocks[block_row * LORES_BLOCK_COLS + block_col];
            uint16_t rgb565 = lores_color_to_rgb565(color_index);

            int px_row_base = block_row * 4; /* each block is 4px tall */
            int px_col_base = block_col * 7; /* each block is 7px wide */
            for (int dy = 0; dy < 4; dy++) {
                int fb_row_base = (px_row_base + dy) * BIO_DISPLAY_WIDTH;
                for (int dx = 0; dx < 7; dx++) {
                    framebuffer[fb_row_base + px_col_base + dx] = rgb565;
                }
            }
        }
    }
}

void bio_display_render_lores_frame_mixed(int page2, int mixed_mode, read6502_fn read_mem,
                                           uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]) {
    if (!read_mem || !framebuffer) {
        return;
    }

    uint8_t blocks[LORES_BLOCK_COLS * LORES_BLOCK_ROWS];
    lores_decode_screen_page(page2, read_mem, blocks);

    int max_block_rows = mixed_mode ? 40 : LORES_BLOCK_ROWS;

    for (int block_row = 0; block_row < max_block_rows; block_row++) {
        for (int block_col = 0; block_col < LORES_BLOCK_COLS; block_col++) {
            uint8_t color_index = blocks[block_row * LORES_BLOCK_COLS + block_col];
            uint16_t rgb565 = lores_color_to_rgb565(color_index);

            int px_row_base = block_row * 4; /* each block is 4px tall */
            int px_col_base = block_col * 7; /* each block is 7px wide */
            for (int dy = 0; dy < 4; dy++) {
                int fb_row_base = (px_row_base + dy) * BIO_DISPLAY_WIDTH;
                for (int dx = 0; dx < 7; dx++) {
                    framebuffer[fb_row_base + px_col_base + dx] = rgb565;
                }
            }
        }
    }
}

void bio_display_render_frame_auto(int hires_mode, int page2, int mixed_mode,
                                    read6502_fn read_mem,
                                    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]) {
    if (hires_mode) {
        bio_display_render_frame_mixed(page2, mixed_mode, read_mem, framebuffer);
    } else {
        bio_display_render_lores_frame_mixed(page2, mixed_mode, read_mem, framebuffer);
    }
}

void bio_display_render_frame_auto_text_aware(int hires_mode, int page2, int mixed_mode,
                                               int text_mode, read6502_fn read_mem,
                                               uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]) {
    if (text_mode && !mixed_mode) {
        /* Full-screen TEXT: no character-ROM renderer exists yet. Fill
         * black rather than leaving stale content from a previous
         * frame's graphics (unlike MIXED mode, nothing else re-fills
         * this region every frame). */
        if (!framebuffer) {
            return; /* safe no-op on bad input */
        }
        for (int i = 0; i < BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT; i++) {
            framebuffer[i] = 0x0000;
        }
        return;
    }
    bio_display_render_frame_auto(hires_mode, page2, mixed_mode, read_mem, framebuffer);
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
