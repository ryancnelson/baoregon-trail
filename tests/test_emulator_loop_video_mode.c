/*
 * test_emulator_loop_video_mode.c -- RED test for a real integration gap:
 * baoregon_emulator_run_frame() never consulted
 * apple2_mem_is_hires_mode(), so every frame rendered Hi-Res regardless
 * of the current LORES/HIRES soft-switch state -- even though
 * bio_display_render_frame_auto() (mode-aware dispatch) already existed
 * for exactly this purpose and real Apple II hardware defaults to LORES
 * post-reset (apple2_mem_reset()'s own doc comment).
 */
#include <stdio.h>
#include <assert.h>

#include "../src/emulator_loop.h"
#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/lores_apple2.h"
#include "../src/video_apple2.h"
#include "../src/bio_display.h"

static void test_run_frame_renders_lores_when_lores_mode_active(void) {
    baoregon_emulator_init();
    /* apple2_mem_reset() defaults to TEXT mode (real Apple II post-reset
     * state) with LORES also selected underneath -- but TEXT mode is a
     * safe no-op now (bio_display_render_frame_auto_text_aware() fix),
     * so GRAPHICS mode must be explicitly selected first to actually see
     * Lo-Res graphics render. */
    assert(apple2_mem_is_hires_mode() == 0);
    write6502(0xC050, 0x00); /* select GRAPHICS mode */
    assert(apple2_mem_is_text_mode() == 0);

    /* Write a known Lo-Res byte to Page 1 row 0 col 0 via the REAL bus
     * (not a mock) so this proves the actual wiring, matching the
     * pattern used in test_video_apple2_realbus.c. */
    write6502(LORES_PAGE1_BASE_ADDR, 0x0F); /* top nibble 0xF = White */

    baoregon_emulator_run_frame();

    const uint16_t *fb = baoregon_emulator_get_framebuffer();
    uint16_t expected_white = lores_color_to_rgb565(0x0F);
    if (fb[0] != expected_white) {
        fprintf(stderr,
                "FAIL: framebuffer[0] = 0x%04X, expected 0x%04X (Lo-Res White) "
                "-- run_frame did not honor LORES mode\n",
                fb[0], expected_white);
        assert(0);
    }
    printf("PASS: test_run_frame_renders_lores_when_lores_mode_active\n");
}

static void test_run_frame_renders_hires_when_hires_mode_active(void) {
    baoregon_emulator_init();

    /* Real Apple II: $C057 write/read selects HIRES mode; $C050 selects
     * GRAPHICS mode (required now that full TEXT mode is correctly a
     * no-op post-reset). */
    write6502(0xC050, 0x00);
    write6502(0xC057, 0x00);
    assert(apple2_mem_is_hires_mode() == 1);
    assert(apple2_mem_is_text_mode() == 0);

    write6502(HIRES_BASE_ADDR, 0x01); /* col0 lit, isolated, even col, palette 0 -> GREEN */

    baoregon_emulator_run_frame();

    const uint16_t *fb = baoregon_emulator_get_framebuffer();
    uint16_t expected_green = bio_display_color_to_rgb565(HIRES_COLOR_GREEN);
    if (fb[0] != expected_green) {
        fprintf(stderr,
                "FAIL: framebuffer[0] = 0x%04X, expected 0x%04X (Hi-Res GREEN)\n",
                fb[0], expected_green);
        assert(0);
    }
    printf("PASS: test_run_frame_renders_hires_when_hires_mode_active\n");
}

int main(void) {
    test_run_frame_renders_lores_when_lores_mode_active();
    test_run_frame_renders_hires_when_hires_mode_active();
    printf("All tests passed.\n");
    return 0;
}
