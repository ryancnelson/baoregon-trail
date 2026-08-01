/*
 * emulator_loop.c -- Full frame-driven Apple II emulator loop implementation.
 */
#include "emulator_loop.h"
#include "apple2_mem.h"
#include "cpu6502.h"
#include "boot_splash.h"
#include "disk_trap.h"
#include "video_apple2.h"
#include "bio_display.h"
#include "bunnie_audio.h"

static boot_splash_state_t g_splash_state;
static boot_splash_button_edge_state_t g_edge_state;
static int g_in_splash_menu = 1;
static uint16_t g_framebuffer[320 * 240];
static uint64_t g_total_cycles = 0ULL;

/* Shared implementation for both public "return to splash menu with
 * fresh state" entry points (baoregon_emulator_init(), called by the
 * 3-button-combo hardware trigger, and baoregon_emulator_reset_to_splash(),
 * used by other callers/tests) -- kept as ONE body so the two can never
 * drift out of sync again. They have already diverged twice in practice
 * (g_total_cycles not zeroed in reset_to_splash() until commit 0f538ac;
 * this refactor closes that class of bug at the source instead of
 * requiring every future new piece of reset state to be manually kept
 * in sync between two separate copy-pasted functions).
 *
 * Real bug fixed here: apple2_mem_reset() zeroes g_button_pressed[]
 * (button register), and boot_splash_button_edge_state_init() assumes
 * "all released" too -- but if a button is STILL PHYSICALLY HELD at
 * the moment of reset (e.g. the 3-button reset combo itself: the user
 * hasn't released PB0/PB1/PB2 yet when init() fires), a real GPIO-
 * polling driver will re-assert pressed=1 for that button on the very
 * next frame. boot_splash's edge detector would then see
 * now_pressed=1, was_pressed=0 (freshly reset) and treat the still-
 * held button as a BRAND NEW press, spuriously firing PREV/NEXT/SELECT
 * in the splash menu the instant it's entered -- even though the user
 * never released and re-pressed anything. Fix: sample the actual
 * pre-reset button state BEFORE apple2_mem_reset() zeroes it, then
 * seed the edge-detection state with those captured values instead of
 * blindly assuming "all released". */
static void reset_to_splash_menu(void) {
    int pb0_was_held = apple2_mem_get_button_state(0);
    int pb1_was_held = apple2_mem_get_button_state(1);
    int pb2_was_held = apple2_mem_get_button_state(2);

    apple2_mem_reset();
    reset6502();
    boot_splash_init(&g_splash_state);
    boot_splash_button_edge_state_init(&g_edge_state);
    g_edge_state.was_pressed[0] = pb0_was_held;
    g_edge_state.was_pressed[1] = pb1_was_held;
    g_edge_state.was_pressed[2] = pb2_was_held;
    g_in_splash_menu = 1;
    g_total_cycles = 0ULL;
}

void baoregon_emulator_init(void) {
    reset_to_splash_menu();
}

uint64_t baoregon_emulator_get_total_cycles(void) {
    return g_total_cycles;
}

int baoregon_emulator_is_in_splash_menu(void) {
    return g_in_splash_menu;
}

void baoregon_emulator_reset_to_splash(void) {
    reset_to_splash_menu();
}

void baoregon_emulator_poll_input(void) {
    if (g_in_splash_menu) {
        int game_selected = boot_splash_poll_apple2_mem_buttons(&g_splash_state, &g_edge_state, disk_trap_set_image);
        if (game_selected) {
            g_in_splash_menu = 0;
            reset6502();
        }
    } else {
        /* Check for 3-button soft reset combo (PB0 + PB1 + PB2 held simultaneously) */
        if (apple2_mem_get_button_state(0) && apple2_mem_get_button_state(1) && apple2_mem_get_button_state(2)) {
            baoregon_emulator_init();
        }
    }
}

uint32_t baoregon_emulator_run_frame(void) {
    baoregon_emulator_poll_input();

    uint32_t start_ticks = clockticks6502;
    if (!g_in_splash_menu) {
        exec6502(BAOREGON_CYCLES_PER_FRAME);
    }
    uint32_t cycles_this_frame = clockticks6502 - start_ticks;
    g_total_cycles += cycles_this_frame;

    /* Process audio state update */
    bunnie_audio_poll_and_apply(apple2_mem_get_audio_state());

    /* Render video frame based on current softswitch modes -- picks
     * HIRES vs LORES via bio_display_render_frame_auto_text_aware()
     * (which itself honors MIXED-mode's text-region boundary for the
     * HIRES/LORES paths, AND is a safe no-op in full TEXT mode -- real
     * Apple II defaults to TEXT mode post-reset, not graphics, and
     * rendering graphics garbage there was a real bug this fixes). */
    int is_hires = apple2_mem_is_hires_mode();
    int is_page2 = apple2_mem_is_page2_selected();
    int is_mixed = apple2_mem_is_mixed_mode();
    int is_text = apple2_mem_is_text_mode();

    bio_display_render_frame_auto_text_aware(is_hires, is_page2, is_mixed, is_text, read6502, g_framebuffer);

    return BAOREGON_CYCLES_PER_FRAME;
}

const uint16_t *baoregon_emulator_get_framebuffer(void) {
    return g_framebuffer;
}

int baoregon_emulator_copy_framebuffer(uint16_t *dest, size_t dest_count) {
    if (!dest || dest_count < (320 * 240)) {
        return -1;
    }
    for (size_t i = 0; i < (320 * 240); i++) {
        dest[i] = g_framebuffer[i];
    }
    return 0;
}

const cartridge_slot_t *baoregon_emulator_get_current_slot(void) {
    return boot_splash_current_slot(&g_splash_state);
}

int baoregon_emulator_is_audio_active(void) {
    const bunnie_audio_state_t *state = apple2_mem_get_audio_state();
    return state ? (int)state->pwm_pin_state : 0;
}

uint32_t baoregon_emulator_get_cycles_per_frame(void) {
    return BAOREGON_CYCLES_PER_FRAME;
}
