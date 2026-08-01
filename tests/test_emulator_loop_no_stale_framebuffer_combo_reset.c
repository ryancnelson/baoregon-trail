/*
 * RED test: verifies the 3-button-combo reset path (PB0+PB1+PB2 held
 * simultaneously -> baoregon_emulator_poll_input() -> baoregon_emulator_init())
 * has the same no-stale-framebuffer property that
 * test_emulator_loop_no_stale_framebuffer.c already proved for
 * baoregon_emulator_reset_to_splash(). That existing test's own comment
 * claims to cover "the 3-button combo -> baoregon_emulator_init()" but
 * never actually exercises that path -- it only calls
 * baoregon_emulator_reset_to_splash() directly. Since baoregon_emulator_init()
 * and baoregon_emulator_reset_to_splash() are separate function bodies
 * (currently identical, but nothing enforces they stay that way), this
 * closes the real verification gap: prove the ACTUAL combo-triggered
 * path is also stale-framebuffer-free, not just its sibling function.
 */
#include <assert.h>
#include <stdio.h>

#include "../src/emulator_loop.h"
#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/video_apple2.h"
#include "../src/lores_apple2.h"
#include "../src/bio_display.h"

static void test_framebuffer_is_fresh_after_3button_combo_reset(void) {
    baoregon_emulator_init();

    /* Enter game via SELECT (PB2), render real Hi-Res content. */
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(2, 0);
    baoregon_emulator_poll_input();
    assert(baoregon_emulator_is_in_splash_menu() == 0);

    write6502(0xC057, 0x00); /* select HIRES */
    write6502(HIRES_BASE_ADDR, 0x01); /* col0 lit -> GREEN */
    baoregon_emulator_run_frame();

    const uint16_t *fb_during_game = baoregon_emulator_get_framebuffer();
    uint16_t expected_green = bio_display_color_to_rgb565(HIRES_COLOR_GREEN);
    assert(fb_during_game[0] == expected_green); /* sanity: really rendered Hi-Res */

    /* Trigger the ACTUAL 3-button-combo reset path -- hold all three
     * buttons simultaneously and poll, which is what real hardware
     * fires through baoregon_emulator_poll_input()'s combo check,
     * calling baoregon_emulator_init() (NOT reset_to_splash()). */
    apple2_mem_set_button_state(0, 1);
    apple2_mem_set_button_state(1, 1);
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(0, 0);
    apple2_mem_set_button_state(1, 0);
    apple2_mem_set_button_state(2, 0);

    assert(baoregon_emulator_is_in_splash_menu() == 1);

    /* The very next run_frame() must show fresh state, NOT the stale
     * Hi-Res GREEN pixel from the previous game session. */
    baoregon_emulator_run_frame();
    const uint16_t *fb_after_reset = baoregon_emulator_get_framebuffer();

    if (fb_after_reset[0] == expected_green) {
        fprintf(stderr, "FAIL: framebuffer[0] still shows stale Hi-Res GREEN "
                        "after 3-button-combo reset (baoregon_emulator_init() "
                        "path) -- real staleness bug in the actual hardware "
                        "reset trigger, not just its sibling function\n");
        assert(0);
    }

    printf("PASS: test_framebuffer_is_fresh_after_3button_combo_reset\n");
}

int main(void) {
    test_framebuffer_is_fresh_after_3button_combo_reset();
    printf("All tests passed.\n");
    return 0;
}
