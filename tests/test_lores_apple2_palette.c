#include <stdio.h>

#include "../src/lores_apple2.h"

/*
 * RED test: Lo-Res 4-bit color index -> RGB565 palette lookup, mirroring
 * the pattern already established for Hi-Res in bio_display.c /
 * test_bio_display.c. Standard Apple II Lo-Res 16-color palette order
 * (documented consistently across every Apple II reference/emulator):
 *   0 Black, 1 Deep Red, 2 Dark Blue, 3 Purple, 4 Dark Green, 5 Gray1,
 *   6 Medium Blue, 7 Light Blue, 8 Brown, 9 Orange, 10 Gray2, 11 Pink,
 *   12 Green, 13 Yellow, 14 Aqua, 15 White.
 */

static int test_black_maps_to_rgb565_zero(void) {
    uint16_t rgb = lores_color_to_rgb565(0);
    if (rgb != 0x0000) {
        fprintf(stderr, "FAIL: color 0 (Black) -> 0x%04X, expected 0x0000\n", rgb);
        return 1;
    }
    printf("PASS: test_black_maps_to_rgb565_zero\n");
    return 0;
}

static int test_white_maps_to_rgb565_max(void) {
    uint16_t rgb = lores_color_to_rgb565(15);
    if (rgb != 0xFFFF) {
        fprintf(stderr, "FAIL: color 15 (White) -> 0x%04X, expected 0xFFFF\n", rgb);
        return 1;
    }
    printf("PASS: test_white_maps_to_rgb565_max\n");
    return 0;
}

static int test_all_16_colors_produce_distinct_values(void) {
    /* Every one of the 16 standard colors must map to a DIFFERENT RGB565
     * value -- if two indices collided, the palette table would have a
     * copy-paste bug (silently merging two visually distinct Apple II
     * colors on screen). */
    uint16_t seen[16];
    for (int i = 0; i < 16; i++) {
        seen[i] = lores_color_to_rgb565((uint8_t)i);
    }
    for (int i = 0; i < 16; i++) {
        for (int j = i + 1; j < 16; j++) {
            if (seen[i] == seen[j]) {
                fprintf(stderr,
                        "FAIL: color %d and color %d both map to 0x%04X (palette collision)\n",
                        i, j, seen[i]);
                return 1;
            }
        }
    }
    printf("PASS: test_all_16_colors_produce_distinct_values\n");
    return 0;
}

static int test_out_of_range_color_returns_black(void) {
    uint16_t rgb = lores_color_to_rgb565(200); /* clearly invalid index */
    if (rgb != 0x0000) {
        fprintf(stderr, "FAIL: out-of-range color -> 0x%04X, expected 0x0000 (black fallback)\n", rgb);
        return 1;
    }
    printf("PASS: test_out_of_range_color_returns_black\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_black_maps_to_rgb565_zero();
    failures += test_white_maps_to_rgb565_max();
    failures += test_all_16_colors_produce_distinct_values();
    failures += test_out_of_range_color_returns_black();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
