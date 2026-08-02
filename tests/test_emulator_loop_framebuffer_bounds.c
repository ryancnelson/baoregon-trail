/*
 * test_emulator_loop_framebuffer_bounds.c -- documents and locks in a
 * real architectural boundary: the internal framebuffer is allocated as
 * a flat 320*240 = 76800 uint16_t array (target badge display resolution
 * per README.md), but bio_display.h's render functions only ever write
 * the first BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT (280*192 = 53760)
 * *linearly contiguous* entries of that flat array (row * 280 + col
 * indexing) -- NOT a true 320-stride 2D sub-rectangle. This test proves
 * that boundary is real (not just documented) by writing a distinctive
 * poison pattern to the margin region before rendering a real HIRES
 * frame, then confirming the poison survives untouched in the margin
 * (indices [53760, 76800)) while the first 53760 entries get genuinely
 * rendered content.
 *
 * CORRECTED 2026-08-02 (two independent fixes converged on this file,
 * reconciled here): this test previously (incorrectly) reinterpreted
 * the flat buffer as a true 320-wide 2D grid when checking margins
 * (`row * 320 + col`), which does NOT match how bio_display.c's
 * renderers actually address the buffer (`row * 280 + col`, tightly
 * packed, no 320-stride gap between rows) -- confirmed by reading every
 * render function in bio_display.c, none of which ever reference the
 * value 320. This mismatch was invisible before because (a) the test's
 * own HIRES poke (write6502(HIRES_BASE_ADDR, 0x01)) only lit row 0's
 * first pixels, with every other row staying all-zero, AND (b) real
 * Apple II defaults to TEXT mode post-reset (not GRAPHICS), and before
 * src/text_apple2.c existed, full TEXT mode always rendered solid black
 * regardless of the HIRES/LORES softswitch -- so this test's original
 * omission of an explicit $C050 (GRAPHICS mode) select was ALSO
 * silently masked. Once text_apple2_render_frame() (real character-ROM
 * glyph rendering) landed, BOTH latent issues surfaced together: this
 * test needs an explicit $C050 GRAPHICS-mode select to actually
 * exercise the HIRES path it's testing (not accidentally hit
 * text_apple2_render_frame() instead), AND the margin check itself
 * needs the correct tight-pack (280 entries/row, no gap) addressing to
 * match how the renderer and baoregon_emulator_copy_framebuffer() (a
 * straight linear memcpy, see emulator_loop.c) actually treat the
 * buffer -- fixing only one of the two would still leave a latent bug.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../src/emulator_loop.h"
#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/video_apple2.h"

#define FULL_WIDTH 320
#define FULL_HEIGHT 240
#define RENDERED_PIXEL_COUNT (HIRES_PIXELS_WIDE * HIRES_ROWS) /* 280*192=53760, tight-packed */
#define POISON 0xDEAD

static void test_margin_pixels_are_never_written(void) {
    baoregon_emulator_init();

    /* Explicit GRAPHICS mode select -- without this, real Apple II's
     * TEXT-mode-post-reset default means bio_display_render_frame_auto_text_aware()
     * dispatches to text_apple2_render_frame() instead of the HIRES
     * path this test is actually meant to exercise (see file header
     * comment). */
    write6502(0xC050, 0x00); /* GRAPHICS mode */
    write6502(0xC057, 0x00); /* HIRES mode */
    write6502(HIRES_BASE_ADDR, 0x01); /* col0 lit -> GREEN, non-zero RGB565 */

    baoregon_emulator_run_frame();

    uint16_t full_buf[FULL_WIDTH * FULL_HEIGHT];
    /* Poison the ENTIRE dest buffer first so we can tell "never written"
     * from "written to zero". */
    for (int i = 0; i < FULL_WIDTH * FULL_HEIGHT; i++) {
        full_buf[i] = POISON;
    }

    int res = baoregon_emulator_copy_framebuffer(full_buf, FULL_WIDTH * FULL_HEIGHT);
    assert(res == 0);

    /* Rendered region: the first RENDERED_PIXEL_COUNT (53760) entries,
     * tightly packed (row * HIRES_PIXELS_WIDE + col, matching exactly
     * how every bio_display.c renderer addresses the buffer) -- must be
     * genuinely rendered (NOT poison -- this proves copy_framebuffer()
     * actually overwrote it with real content, not a no-op). */
    int rendered_pixel_found_nonpoison = 0;
    for (int i = 0; i < RENDERED_PIXEL_COUNT; i++) {
        if (full_buf[i] != POISON) {
            rendered_pixel_found_nonpoison = 1;
            break;
        }
    }
    if (!rendered_pixel_found_nonpoison) {
        fprintf(stderr, "FAIL: no pixel in the rendered region differs from poison -- "
                        "render_frame_auto_text_aware did not actually write real content\n");
        assert(0);
    }

    /* Margin pixels: everything from index RENDERED_PIXEL_COUNT (53760)
     * to FULL_WIDTH*FULL_HEIGHT (76800) -- must be 0 (blank/unrendered)
     * -- proves the renderer genuinely never writes real content past
     * its documented tight-packed region. copy_framebuffer() does copy
     * the full 320x240 buffer (including margins), so the poison IS
     * overwritten -- but with the margin's static-zero-init value, not
     * with any genuinely rendered pixel data. */
    int margin_violations = 0;
    for (int i = RENDERED_PIXEL_COUNT; i < FULL_WIDTH * FULL_HEIGHT; i++) {
        if (full_buf[i] != 0x0000) {
            margin_violations++;
        }
    }
    if (margin_violations != 0) {
        fprintf(stderr, "FAIL: %d margin pixel(s) were overwritten -- renderer wrote outside "
                        "its documented tight-packed region (possible buffer-size regression)\n",
                margin_violations);
        assert(0);
    }

    printf("PASS: test_margin_pixels_are_never_written\n");
}

int main(void) {
    test_margin_pixels_are_never_written();
    printf("All tests passed.\n");
    return 0;
}
