/*
 * tests/test_emulator_loop_copy_framebuffer.c -- Unit test for baoregon_emulator_copy_framebuffer.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "emulator_loop.h"
#include "bio_display.h"

static void test_copy_framebuffer_null_and_bounds_safety(void) {
    baoregon_emulator_init();
    baoregon_emulator_run_frame();

    /* NULL dest should return -1 */
    int res1 = baoregon_emulator_copy_framebuffer(NULL, 320 * 240);
    assert(res1 == -1);

    /* Under-sized dest_count should return -1 */
    uint16_t small_buf[100];
    int res2 = baoregon_emulator_copy_framebuffer(small_buf, 100);
    assert(res2 == -1);

    /* Valid dest should return 0 and match get_framebuffer() content in
     * the native BIO_DISPLAY_WIDTH x BIO_DISPLAY_HEIGHT (280x192)
     * region -- comparing the FULL 320x240 buffer against
     * get_framebuffer()'s raw pointer is a real out-of-bounds read:
     * g_framebuffer is only allocated at its native 280x192 size (see
     * emulator_loop.c), not the full 320x240 destination size. Only
     * copy_framebuffer()'s OWN dest buffer is padded/zeroed to 320x240
     * -- the source it copies FROM is not, and never claims to be
     * (baoregon_emulator_get_framebuffer()'s doc comment only promises
     * the native resolution). This bug was previously masked by
     * whatever happened to sit in adjacent memory after g_framebuffer;
     * it surfaced as a real memcmp mismatch once Step 9's text_apple2.c
     * static buffers shifted the binary's memory layout. */
    uint16_t full_buf[320 * 240];
    memset(full_buf, 0, sizeof(full_buf));
    int res3 = baoregon_emulator_copy_framebuffer(full_buf, 320 * 240);
    assert(res3 == 0);

    const uint16_t *src = baoregon_emulator_get_framebuffer();
    for (int row = 0; row < BIO_DISPLAY_HEIGHT; row++) {
        int dest_offset = row * 320;
        int src_offset = row * BIO_DISPLAY_WIDTH;
        assert(memcmp(&full_buf[dest_offset], &src[src_offset], BIO_DISPLAY_WIDTH * sizeof(uint16_t)) == 0);
    }

    printf("PASS: test_copy_framebuffer_null_and_bounds_safety\n");
}

int main(void) {
    test_copy_framebuffer_null_and_bounds_safety();
    return 0;
}
