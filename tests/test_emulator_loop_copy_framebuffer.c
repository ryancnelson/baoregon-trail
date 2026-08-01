/*
 * tests/test_emulator_loop_copy_framebuffer.c -- Unit test for baoregon_emulator_copy_framebuffer.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "emulator_loop.h"

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

    /* Valid dest should return 0 and match get_framebuffer() content */
    uint16_t full_buf[320 * 240];
    memset(full_buf, 0, sizeof(full_buf));
    int res3 = baoregon_emulator_copy_framebuffer(full_buf, 320 * 240);
    assert(res3 == 0);

    const uint16_t *src = baoregon_emulator_get_framebuffer();
    assert(memcmp(full_buf, src, sizeof(full_buf)) == 0);

    printf("PASS: test_copy_framebuffer_null_and_bounds_safety\n");
}

int main(void) {
    test_copy_framebuffer_null_and_bounds_safety();
    return 0;
}
