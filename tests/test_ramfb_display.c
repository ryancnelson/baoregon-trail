/*
 * tests/test_ramfb_display.c -- unit test for ramfb_display driver.
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include "../src/ramfb_display.h"

int main(void) {
    /* Initialize ramfb driver */
    assert(ramfb_init() == 0);

    const uint32_t *buf = ramfb_get_buffer();
    assert(buf != NULL);
    assert(buf[0] == 0xFF000000u);

    /* Pixel conversion tests */
    assert(ramfb_pixel_565_to_xrgb8888(0x0000) == 0xFF000000u); /* Black */
    assert(ramfb_pixel_565_to_xrgb8888(0xFFFF) == 0xFFFFFFFFu); /* White */
    assert(ramfb_pixel_565_to_xrgb8888(0xF800) == 0xFFFF0000u); /* Pure Red */
    assert(ramfb_pixel_565_to_xrgb8888(0x07E0) == 0xFF00FF00u); /* Pure Green */
    assert(ramfb_pixel_565_to_xrgb8888(0x001F) == 0xFF0000FFu); /* Pure Blue */

    /* Frame conversion test */
    uint16_t src[RAMFB_WIDTH * RAMFB_HEIGHT];
    for (size_t i = 0; i < (RAMFB_WIDTH * RAMFB_HEIGHT); i++) {
        src[i] = 0xF800; /* All Red */
    }

    ramfb_present_frame565(src);
    assert(buf[0] == 0xFFFF0000u);
    assert(buf[(RAMFB_WIDTH * RAMFB_HEIGHT) - 1] == 0xFFFF0000u);

    /* NULL safety test */
    ramfb_present_frame565(NULL);
    assert(buf[0] == 0xFFFF0000u); /* Unchanged */

    printf("PASS: ramfb_display verified\n");
    printf("All tests passed.\n");
    return 0;
}
