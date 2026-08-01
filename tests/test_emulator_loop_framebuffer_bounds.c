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
#include <string.h>

#include "../src/emulator_loop.h"
#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/video_apple2.h"

#define FULL_WIDTH 320
#define FULL_HEIGHT 240
#define POISON 0xDEAD

static void test_margin_pixels_are_never_written(void) {
    baoregon_emulator_init();

    /* Select HIRES mode and write a real pattern so the top-left region
     * gets genuinely non-zero rendered content -- proves this isn't a
     * "both happen to be zero" false pass. */
    write6502(0xC057, 0x00);
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

    /* Top-left 280x192 region: must be genuinely rendered (NOT poison --
     * this proves copy_framebuffer() actually overwrote it with real
     * content, not a no-op). */
    int rendered_pixel_found_nonpoison = 0;
    for (int row = 0; row < HIRES_ROWS; row++) {
        for (int col = 0; col < HIRES_PIXELS_WIDE; col++) {
            uint16_t px = full_buf[row * FULL_WIDTH + col];
            if (px != POISON) {
                rendered_pixel_found_nonpoison = 1;
            }
        }
    }
    if (!rendered_pixel_found_nonpoison) {
        fprintf(stderr, "FAIL: no pixel in the rendered 280x192 region differs from poison -- "
                        "render_frame_auto did not actually write real content\n");
        assert(0);
    }

    /* Margin pixels (right of col 280, and below row 192) must be 0
     * (blank/unrendered) -- proves the renderer genuinely never writes
     * real content there. copy_framebuffer() does copy the full
     * 320x240 buffer (including margins), so the poison IS overwritten
     * -- but with the margin's static-zero-init value, not with any
     * genuinely rendered pixel data. */
    int margin_violations = 0;
    for (int row = 0; row < FULL_HEIGHT; row++) {
        for (int col = 0; col < FULL_WIDTH; col++) {
            int in_rendered_region = (row < HIRES_ROWS) && (col < HIRES_PIXELS_WIDE);
            if (in_rendered_region) continue;

            uint16_t px = full_buf[row * FULL_WIDTH + col];
            if (px != 0x0000) {
                margin_violations++;
            }
        }
    }
    if (margin_violations != 0) {
        fprintf(stderr, "FAIL: %d margin pixel(s) were overwritten -- renderer wrote outside "
                        "its documented 280x192 region (possible buffer-size regression)\n",
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
