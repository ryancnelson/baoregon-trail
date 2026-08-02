#include <stdio.h>
#include <string.h>

#include "../src/bio_display.h"
#include "../src/video_apple2.h"

/*
 * RED test (vertical tracer bullet): prove the hires_color_t -> RGB565
 * palette lookup for one known case (BLACK), then the full-frame render
 * pipeline end-to-end against a mock read6502.
 */

static uint8_t g_mock_frame[8192];

static uint8_t mock_read6502(uint16_t address) {
    uint16_t offset = address - HIRES_BASE_ADDR;
    if (offset < sizeof(g_mock_frame)) {
        return g_mock_frame[offset];
    }
    return 0x00;
}

static int test_black_maps_to_rgb565_zero(void) {
    uint16_t rgb = bio_display_color_to_rgb565(HIRES_COLOR_BLACK);
    if (rgb != 0x0000) {
        fprintf(stderr, "FAIL: BLACK -> 0x%04X, expected 0x0000\n", rgb);
        return 1;
    }
    printf("PASS: test_black_maps_to_rgb565_zero\n");
    return 0;
}

static int test_white_maps_to_rgb565_max(void) {
    uint16_t rgb = bio_display_color_to_rgb565(HIRES_COLOR_WHITE);
    if (rgb != 0xFFFF) {
        fprintf(stderr, "FAIL: WHITE -> 0x%04X, expected 0xFFFF\n", rgb);
        return 1;
    }
    printf("PASS: test_white_maps_to_rgb565_max\n");
    return 0;
}

static int test_render_frame_decodes_row0_pixel0_to_correct_rgb565(void) {
    memset(g_mock_frame, 0x00, sizeof(g_mock_frame));
    g_mock_frame[0] = 0x01; /* row 0 byte 0 bit0 set -> col0 lit, isolated,
                             * even col, palette 0 -> GREEN per BRAINSTORM
                             * sec 2 step 4 / video_apple2.h rule. */

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer)); /* poison */

    bio_display_render_frame(mock_read6502, framebuffer);

    uint16_t expected_green = bio_display_color_to_rgb565(HIRES_COLOR_GREEN);
    if (framebuffer[0] != expected_green) {
        fprintf(stderr, "FAIL: framebuffer[0] = 0x%04X, expected 0x%04X (GREEN)\n",
                framebuffer[0], expected_green);
        return 1;
    }
    printf("PASS: test_render_frame_decodes_row0_pixel0_to_correct_rgb565\n");
    return 0;
}

static int test_render_frame_fills_every_pixel_no_poison_left(void) {
    memset(g_mock_frame, 0x00, sizeof(g_mock_frame));

    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0xAA, sizeof(framebuffer));

    bio_display_render_frame(mock_read6502, framebuffer);

    uint16_t expected_black = bio_display_color_to_rgb565(HIRES_COLOR_BLACK);
    for (int i = 0; i < BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT; i++) {
        if (framebuffer[i] != expected_black) {
            fprintf(stderr, "FAIL: framebuffer[%d] = 0x%04X, expected 0x%04X (all-black frame)\n",
                    i, framebuffer[i], expected_black);
            return 1;
        }
    }
    printf("PASS: test_render_frame_fills_every_pixel_no_poison_left\n");
    return 0;
}

static int test_dma_push_records_framebuffer_pointer_and_size(void) {
    uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0, sizeof(framebuffer));

    bio_display_dma_push(framebuffer, BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT);

    const uint16_t *recorded_fb = NULL;
    uint32_t recorded_count = 0;
    int was_called = bio_display_last_dma_push(&recorded_fb, &recorded_count);

    if (!was_called) {
        fprintf(stderr, "FAIL: bio_display_last_dma_push reported not-called after a push\n");
        return 1;
    }
    if (recorded_fb != framebuffer) {
        fprintf(stderr, "FAIL: recorded framebuffer pointer mismatch\n");
        return 1;
    }
    if (recorded_count != (uint32_t)(BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT)) {
        fprintf(stderr, "FAIL: recorded pixel_count = %u, expected %d\n",
                recorded_count, BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT);
        return 1;
    }
    printf("PASS: test_dma_push_records_framebuffer_pointer_and_size\n");
    return 0;
}

static int test_dma_push_not_yet_called_reports_false(void) {
    /* This must run before any bio_display_dma_push() call in a fresh
     * process -- since we don't have a reset function yet, this is
     * ordered first in main() below. */
    const uint16_t *recorded_fb = NULL;
    uint32_t recorded_count = 0;
    int was_called = bio_display_last_dma_push(&recorded_fb, &recorded_count);
    if (was_called) {
        fprintf(stderr, "FAIL: expected not-called before any dma_push, got called=1\n");
        return 1;
    }
    printf("PASS: test_dma_push_not_yet_called_reports_false\n");
    return 0;
}

static int test_crt_modes(void) {
    bio_display_set_crt_mode(BIO_CRT_MODE_COLOR);
    if (bio_display_get_crt_mode() != BIO_CRT_MODE_COLOR) return 1;

    bio_display_set_crt_mode(BIO_CRT_MODE_GREEN_PHOSPHOR);
    if (bio_display_get_crt_mode() != BIO_CRT_MODE_GREEN_PHOSPHOR) return 1;
    uint16_t green_rgb = bio_display_color_to_rgb565(HIRES_COLOR_WHITE);
    if ((green_rgb & 0xF81F) != 0 || (green_rgb & 0x07E0) == 0) {
        fprintf(stderr, "FAIL: Green Phosphor white pixel should be pure green, got 0x%04X\n", green_rgb);
        return 1;
    }

    bio_display_set_crt_mode(BIO_CRT_MODE_AMBER);
    if (bio_display_get_crt_mode() != BIO_CRT_MODE_AMBER) return 1;
    uint16_t amber_rgb = bio_display_color_to_rgb565(HIRES_COLOR_WHITE);
    if ((amber_rgb & 0x001F) != 0 || (amber_rgb & 0xF800) == 0) {
        fprintf(stderr, "FAIL: Amber CRT white pixel should have red/green, zero blue, got 0x%04X\n", amber_rgb);
        return 1;
    }

    bio_display_set_crt_mode(BIO_CRT_MODE_COLOR); /* restore default */
    printf("PASS: test_crt_modes\n");
    return 0;
}

int main(void) {
    int failures = 0;
    /* Must run first: checks the never-called state before any push. */
    failures += test_dma_push_not_yet_called_reports_false();

    failures += test_black_maps_to_rgb565_zero();
    failures += test_white_maps_to_rgb565_max();
    failures += test_render_frame_decodes_row0_pixel0_to_correct_rgb565();
    failures += test_render_frame_fills_every_pixel_no_poison_left();
    failures += test_dma_push_records_framebuffer_pointer_and_size();
    failures += test_crt_modes();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
