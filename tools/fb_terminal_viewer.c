/*
 * fb_terminal_viewer.c -- host-only terminal viewer for Apple II screen
 * buffers rendered by bio_display.c, per NEXT_STEPS.md Step 4's checklist
 * item: "Write host SDL2 or terminal viewer to verify rendering of Apple
 * II screen buffers." (Not yet done anywhere in this codebase as of
 * 2026-08-xx -- this closes that gap without an SDL2 dependency, using
 * ANSI 24-bit truecolor half-block rendering instead, which every modern
 * terminal supports and needs zero extra libraries.)
 *
 * Reads a RAW RGB565 framebuffer (BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT
 * uint16_t values, little-endian, exactly what bio_display_render_frame*()
 * writes) from a file and prints it to the terminal using the Unicode
 * upper-half-block character (U+2580) with independently colored
 * foreground (top pixel) and background (bottom pixel) -- packs 2 Apple
 * II scanlines into every 1 terminal row, so a 280x192 frame renders as
 * 280 columns x 96 terminal rows (still needs a wide terminal, but no
 * further scaling is invented here -- this is a diagnostic tool, not the
 * real hardware display path).
 *
 * Usage: fb_terminal_viewer <raw_rgb565_file>
 *
 * To generate a raw framebuffer file, dump bio_display_render_frame()'s
 * output with a small harness (see tools/dump_framebuffer.c) or any test
 * that has a `uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]`
 * -- fwrite() it directly, this tool doesn't care about the source.
 */
#include <stdio.h>
#include <stdlib.h>

#include "../src/bio_display.h"

void fb_terminal_viewer_print(const uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]);

static void print_rgb565_as_ansi_fg(uint16_t rgb565) {
    uint8_t r5 = (rgb565 >> 11) & 0x1F;
    uint8_t g6 = (rgb565 >> 5) & 0x3F;
    uint8_t b5 = rgb565 & 0x1F;
    /* Expand 5/6-bit channels to 8-bit for ANSI 24-bit color. */
    uint8_t r8 = (uint8_t)((r5 * 255 + 15) / 31);
    uint8_t g8 = (uint8_t)((g6 * 255 + 31) / 63);
    uint8_t b8 = (uint8_t)((b5 * 255 + 15) / 31);
    printf("\x1b[38;2;%u;%u;%um", r8, g8, b8);
}

static void print_rgb565_as_ansi_bg(uint16_t rgb565) {
    uint8_t r5 = (rgb565 >> 11) & 0x1F;
    uint8_t g6 = (rgb565 >> 5) & 0x3F;
    uint8_t b5 = rgb565 & 0x1F;
    uint8_t r8 = (uint8_t)((r5 * 255 + 15) / 31);
    uint8_t g8 = (uint8_t)((g6 * 255 + 31) / 63);
    uint8_t b8 = (uint8_t)((b5 * 255 + 15) / 31);
    printf("\x1b[48;2;%u;%u;%um", r8, g8, b8);
}

void fb_terminal_viewer_print(const uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]) {
    /* Pack 2 scanlines per terminal row using U+2580 (upper half block):
     * foreground color = top pixel, background color = bottom pixel.
     * BIO_DISPLAY_HEIGHT (192) is even, so no leftover odd row. */
    for (int row_pair = 0; row_pair < BIO_DISPLAY_HEIGHT; row_pair += 2) {
        for (int col = 0; col < BIO_DISPLAY_WIDTH; col++) {
            uint16_t top = framebuffer[row_pair * BIO_DISPLAY_WIDTH + col];
            uint16_t bottom = framebuffer[(row_pair + 1) * BIO_DISPLAY_WIDTH + col];
            print_rgb565_as_ansi_fg(top);
            print_rgb565_as_ansi_bg(bottom);
            printf("\xe2\x96\x80"); /* UTF-8 encoding of U+2580 UPPER HALF BLOCK */
        }
        printf("\x1b[0m\n"); /* reset colors at end of each terminal row */
    }
}

#ifndef FB_TERMINAL_VIEWER_NO_MAIN
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <raw_rgb565_file>\n", argv[0]);
        fprintf(stderr, "  file must contain exactly %d uint16_t values "
                        "(%d bytes), row-major, matching bio_display.h's "
                        "BIO_DISPLAY_WIDTH x BIO_DISPLAY_HEIGHT.\n",
                BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT,
                (int)(BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT * sizeof(uint16_t)));
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    static uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    size_t want = BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT;
    size_t got = fread(framebuffer, sizeof(uint16_t), want, f);
    fclose(f);

    if (got != want) {
        fprintf(stderr, "error: read %zu uint16_t values, expected %zu "
                        "(file wrong size for a %dx%d RGB565 framebuffer)\n",
                got, want, BIO_DISPLAY_WIDTH, BIO_DISPLAY_HEIGHT);
        return 1;
    }

    fb_terminal_viewer_print(framebuffer);
    return 0;
}
#endif
