/*
 * sw/lores_palette/main.c -- BIO Core 0 firmware: Lo-Res 4-bit color
 * index -> RGB565 palette lookup, ported from the already
 * host-and-cross-compile verified src/lores_apple2.c
 * (lores_color_to_rgb565()) in baoregon-trail, for REAL execution under
 * bio-sim (github.com/baochip/bio-sim -- Verilator simulation of the
 * actual bio_bdma RTL, not a mock).
 *
 * Protocol (driven by bio-sim-tests/configs/lores_palette.jsonc via
 * fifo_write/fifo_read commands):
 *   1. Host writes one 32-bit color index (0-15, 4-bit Lo-Res palette)
 *      to FIFO0.
 *   2. This program pops it, looks up the RGB565 value using the EXACT
 *      SAME table as src/lores_apple2.c's palette[], and pushes the
 *      result to FIFO1.
 *   3. Loops forever, one color per iteration.
 *
 * This proves the Lo-Res palette logic (already unit-tested +
 * cross-compiled for the main-CPU target in baoregon-trail) also holds
 * when hand-ported to the real BIO-core hardware interface, mirroring
 * the same verification already done for the Hi-Res palette in
 * bio_display_palette/main.c.
 */
#include <stdint.h>

#include "bio.h" // this must always be first

/* Must exactly match src/lores_apple2.c's palette[] in baoregon-trail --
 * same 4-bit index ordinals, same RGB565 constants. Kept in sync
 * manually (no shared header between the two repos yet); see
 * bio-sim-tests/README.md for the cross-check procedure. */
static const uint32_t g_lores_palette[16] = {
    0x0000, /*  0 Black       */
    0x9000, /*  1 Deep Red    */
    0x000D, /*  2 Dark Blue   */
    0xA0B8, /*  3 Purple      */
    0x0320, /*  4 Dark Green  */
    0x738E, /*  5 Gray 1      */
    0x055F, /*  6 Medium Blue */
    0x7BFF, /*  7 Light Blue  */
    0x5300, /*  8 Brown       */
    0xFC60, /*  9 Orange      */
    0xC618, /* 10 Gray 2      */
    0xFB56, /* 11 Pink        */
    0x07E0, /* 12 Green       */
    0xFFA0, /* 13 Yellow      */
    0x7FF5, /* 14 Aqua        */
    0xFFFF, /* 15 White       */
};

void main(void) {
    while (1) {
        uint32_t color_index = pop_fifo0();
        uint32_t rgb565 = 0x0000; /* out-of-range: black fallback, matches lores_color_to_rgb565() */
        if (color_index < 16) {
            rgb565 = g_lores_palette[color_index];
        }
        push_fifo1(rgb565);
    }
}
