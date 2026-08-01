/*
 * sw/bio_display_palette/main.c -- BIO Core 0 firmware: color-index ->
 * RGB565 palette lookup, ported from the already host-and-cross-compile
 * verified src/bio_display.c (bio_display_color_to_rgb565()) in
 * baoregon-trail, for REAL execution under bio-sim
 * (github.com/baochip/bio-sim -- Verilator simulation of the actual
 * bio_bdma RTL, not a mock).
 *
 * Protocol (driven by bio-sim-tests/configs/bio_display_palette.jsonc via
 * fifo_write/fifo_read commands):
 *   1. Host writes one 32-bit color index (0-5, matching hires_color_t)
 *      to FIFO0.
 *   2. This program pops it, looks up the RGB565 value using the EXACT
 *      SAME table as src/bio_display.c's g_color_to_rgb565[], and pushes
 *      the result to FIFO1.
 *   3. Loops forever, one color per iteration -- host can drive as many
 *      lookups as it wants across one sim run.
 *
 * This proves the palette logic (already unit-tested + cross-compiled
 * for the main-CPU target in baoregon-trail) also holds when hand-ported
 * to the real BIO-core hardware interface (FIFO registers via bio.h,
 * -march=rv32imc -mabi=ilp32 with x16-x31 reserved per BRAINSTORM.md's
 * corrected hardware model, 2026-08-01).
 */
#include <stdint.h>

#include "bio.h" // this must always be first

/* Must exactly match src/bio_display.c's g_color_to_rgb565[] in
 * baoregon-trail -- same hires_color_t ordinal values, same RGB565
 * constants. Kept in sync manually (no shared header between the two
 * repos yet); see bio-sim-tests/README.md for the cross-check procedure. */
static const uint32_t g_color_to_rgb565[6] = {
    0x0000, /* HIRES_COLOR_BLACK  */
    0x07E0, /* HIRES_COLOR_GREEN  */
    0x781F, /* HIRES_COLOR_VIOLET */
    0xFC00, /* HIRES_COLOR_ORANGE */
    0x001F, /* HIRES_COLOR_BLUE   */
    0xFFFF, /* HIRES_COLOR_WHITE  */
};

void main(void) {
    while (1) {
        uint32_t color_index = pop_fifo0();
        uint32_t rgb565 = 0xDEAD; /* poison: unknown color index */
        if (color_index < 6) {
            rgb565 = g_color_to_rgb565[color_index];
        }
        push_fifo1(rgb565);
    }
}
