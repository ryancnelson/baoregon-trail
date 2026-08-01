/*
 * test_emulator_loop_dma_push.c -- real integration gap: bio_display.h's
 * bio_display_dma_push() (the BIO Core 0 -> SPI DMA staging stub) was
 * never called anywhere in baoregon_emulator_run_frame(). The frame loop
 * renders into g_framebuffer every frame but never "pushes" it out, so
 * the DMA-push contract (already unit-tested in isolation via
 * bio_display_last_dma_push()) had no real caller path proving the
 * actual frame loop would ever drive it -- a stub with zero real
 * integration coverage. This test proves the gap, then the fix wires
 * bio_display_dma_push() into run_frame() after rendering completes.
 */
#include <assert.h>
#include <stdio.h>

#include "../src/emulator_loop.h"
#include "../src/bio_display.h"

static void test_run_frame_pushes_framebuffer_via_dma(void) {
    baoregon_emulator_init();

    baoregon_emulator_run_frame();

    const uint16_t *pushed_fb = NULL;
    uint32_t pushed_count = 0;
    int was_called = bio_display_last_dma_push(&pushed_fb, &pushed_count);

    if (!was_called) {
        fprintf(stderr, "FAIL: bio_display_dma_push() was never called by "
                        "run_frame() -- real integration gap, the frame loop "
                        "renders but never pushes the result out\n");
        assert(0);
    }

    if (pushed_fb != baoregon_emulator_get_framebuffer()) {
        fprintf(stderr, "FAIL: bio_display_dma_push() was called with the wrong "
                        "framebuffer pointer\n");
        assert(0);
    }

    if (pushed_count != BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT) {
        fprintf(stderr, "FAIL: bio_display_dma_push() pixel_count = %u, expected %u "
                        "(native BIO_DISPLAY_WIDTH*HEIGHT, not the full 320x240 buffer "
                        "-- only the rendered region should be pushed)\n",
                pushed_count, (unsigned)(BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT));
        assert(0);
    }

    printf("PASS: test_run_frame_pushes_framebuffer_via_dma\n");
}

int main(void) {
    test_run_frame_pushes_framebuffer_via_dma();
    printf("All tests passed.\n");
    return 0;
}
