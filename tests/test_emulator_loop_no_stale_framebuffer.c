/*
 * test_emulator_loop_no_stale_framebuffer_after_reset.c -- verifies a
 * real potential staleness bug does NOT exist: after a game renders
 * Hi-Res content and baoregon_emulator_reset_to_splash() (or the
 * 3-button combo -> baoregon_emulator_init()) fires, the very next
 * baoregon_emulator_run_frame() must show fresh LORES splash content,
 * not leftover Hi-Res pixels from the previous game session. This is
 * true by construction (run_frame() always re-renders unconditionally
 * from current apple2_mem state every call), but had no direct test
 * proving it -- adding one closes that gap.
 */
#include <assert.h>
#include <stdio.h>

#include "../src/emulator_loop.h"
#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/video_apple2.h"
#include "../src/lores_apple2.h"
#include "../src/bio_display.h"

static void test_framebuffer_is_fresh_lores_after_reset_to_splash(void) {
    baoregon_emulator_init();

    /* Simulate being "in game" with real Hi-Res content rendered. */
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(2, 0);
    assert(baoregon_emulator_is_in_splash_menu() == 0);

    write6502(0xC057, 0x00); /* select HIRES */
    write6502(HIRES_BASE_ADDR, 0x01); /* col0 lit -> GREEN */
    baoregon_emulator_run_frame();

    const uint16_t *fb_during_game = baoregon_emulator_get_framebuffer();
    uint16_t expected_green = bio_display_color_to_rgb565(HIRES_COLOR_GREEN);
    assert(fb_during_game[0] == expected_green); /* sanity: really rendered Hi-Res */

    /* Now reset to splash -- real Apple II defaults to LORES post-reset. */
    baoregon_emulator_reset_to_splash();
    assert(baoregon_emulator_is_in_splash_menu() == 1);

    /* The very next run_frame() must show fresh state, NOT the stale
     * Hi-Res GREEN pixel from the previous game session. */
    baoregon_emulator_run_frame();
    const uint16_t *fb_after_reset = baoregon_emulator_get_framebuffer();

    if (fb_after_reset[0] == expected_green) {
        fprintf(stderr, "FAIL: framebuffer[0] still shows stale Hi-Res GREEN "
                        "after reset_to_splash() -- real staleness bug\n");
        assert(0);
    }

    printf("PASS: test_framebuffer_is_fresh_lores_after_reset_to_splash\n");
}

int main(void) {
    test_framebuffer_is_fresh_lores_after_reset_to_splash();
    printf("All tests passed.\n");
    return 0;
}
