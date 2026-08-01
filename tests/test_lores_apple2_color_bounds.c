/*
 * tests/test_lores_apple2_color_bounds.c -- unit test verifying
 * lores_color_to_rgb565() color palette indexing for valid colors (0..15)
 * and out-of-bounds inputs (16, 255) returning 0x0000 (black fallback).
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "../src/lores_apple2.h"

int main(void) {
    /* Verify all 16 valid colors produce expected non-zero RGB565 values (except Black 0) */
    assert(lores_color_to_rgb565(0) == 0x0000);  /* Black */
    assert(lores_color_to_rgb565(1) == 0x9000);  /* Deep Red */
    assert(lores_color_to_rgb565(2) == 0x000D);  /* Dark Blue */
    assert(lores_color_to_rgb565(3) == 0xA0B8);  /* Purple */
    assert(lores_color_to_rgb565(4) == 0x0320);  /* Dark Green */
    assert(lores_color_to_rgb565(5) == 0x738E);  /* Gray 1 */
    assert(lores_color_to_rgb565(6) == 0x055F);  /* Medium Blue */
    assert(lores_color_to_rgb565(7) == 0x7BFF);  /* Light Blue */
    assert(lores_color_to_rgb565(8) == 0x5300);  /* Brown */
    assert(lores_color_to_rgb565(9) == 0xFC60);  /* Orange */
    assert(lores_color_to_rgb565(10) == 0xC618); /* Gray 2 */
    assert(lores_color_to_rgb565(11) == 0xFB56); /* Pink */
    assert(lores_color_to_rgb565(12) == 0x07E0); /* Green */
    assert(lores_color_to_rgb565(13) == 0xFFA0); /* Yellow */
    assert(lores_color_to_rgb565(14) == 0x7FF5); /* Aqua */
    assert(lores_color_to_rgb565(15) == 0xFFFF); /* White */

    /* Out of bounds color indices must safely return 0x0000 (black) */
    assert(lores_color_to_rgb565(16) == 0x0000);
    assert(lores_color_to_rgb565(17) == 0x0000);
    assert(lores_color_to_rgb565(255) == 0x0000);

    printf("PASS: lores_color_to_rgb565 color palette and bounds safety verified\n");
    printf("All tests passed.\n");
    return 0;
}
