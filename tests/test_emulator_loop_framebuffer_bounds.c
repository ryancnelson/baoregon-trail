/*
 * test_emulator_loop_framebuffer_bounds.c -- documents and locks in a
 * real architectural boundary: the internal framebuffer is allocated at
 * 320x240 (target badge display resolution per README.md), but
 * bio_display.h's render functions only ever write the native
 * BIO_DISPLAY_WIDTH x BIO_DISPLAY_HEIGHT (280x192) region in the
 * top-left corner -- scaling up to fill 320x240 is explicitly deferred
 * pending baochip's target-resolution confirmation. This test proves
 * that boundary is real (not just documented) by writing a distinctive
 * poison pattern to the margin region before rendering a real Hi-Res
 * frame, then confirming the poison survives untouched in the margins
 * while the top-left 280x192 region gets genuinely rendered content.
 */
#include <assert.h>
#include <stdio.h>
/*
 * test_emulator_loop_framebuffer_bounds.c -- documents and locks in a
 * real architectural boundary: the internal framebuffer is allocated as
 * a flat 320*240 = 76800 uint16_t array (target badge display resolution
 * per README.md), but bio_display.h's render functions only ever write
 * the first BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT (280*192 = 53760)
 * *linearly contiguous* entries of that flat array (row * 280 + col
 * indexing) -- NOT a true 320-stride 2D sub-rectangle. This test proves
 * that boundary is real (not just documented) by writing a distinctive
 * poison pattern to the margin region before rendering a real frame,
 * then confirming the poison survives untouched in the margin (indices
 * [53760, 76800)) while the first 53760 entries get genuinely rendered
 * content.
 *
 * CORRECTED 2026-08-02: this test previously (incorrectly) reinterpreted
 * the flat buffer as a true 320-wide 2D grid when checking margins
 * (`row * 320 + col`), which does NOT match how bio_display.c's
 * renderers actually address the buffer (`row * 280 + col`, tightly
 * packed, no 320-stride gap between rows) -- confirmed by reading every
 * render function in bio_display.c, none of which ever reference the
 * value 320. This mismatch was invisible before because the test's own
 * HIRES poke (write6502(HIRES_BASE_ADDR, 0x01)) only lit row 0's first
 * pixels, and every other row stayed all-zero either way -- so the
 * wrong-stride margin check happened to see zeros regardless of which
 * addressing convention it used. Once text_apple2_render_frame() (real
 * character-ROM glyph rendering, replacing the old
 * always-black-in-TEXT-mode placeholder) started producing genuinely
 * non-zero content in EVERY row -- since this test never explicitly
 * selects GRAPHICS mode ($C050), and real Apple II defaults to TEXT
 * mode post-reset -- the old wrong-stride check started reporting
 * false "margin violations" that don't reflect any real bounds
 * overrun. Fixed to check margins using the SAME tight-pack (280
 * entries/row, no gap) addressing the renderer and
 * baoregon_emulator_copy_framebuffer() (a straight linear memcpy, see
 * emulator_loop.c) both actually use.
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

    /* Select HIRES mode and write a real pattern so the rendered region
     * gets genuinely non-zero content -- proves this isn't a "both
     * happen to be zero" false pass. Real Apple II defaults to TEXT
     * mode post-reset (not GRAPHICS) -- this test deliberately does NOT
     * select GRAPHICS mode either, so bio_display_render_frame_auto_text_aware()
     * actually dispatches to text_apple2_render_frame() (real
     * character-ROM glyph rendering), not the HIRES path -- exercising
     * exactly the code path that originally exposed this test's own
     * addressing bug (see file header comment). The $C057/HIRES write
     * below is kept for historical continuity with the original test's
     * intent but is not what actually renders once TEXT mode wins. */
    write6502(0xC057, 0x00);
    write6502(HIRES_BASE_ADDR, 0x01); /* col0 lit -> GREEN, non-zero RGB565 (irrelevant while in TEXT mode) */

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
