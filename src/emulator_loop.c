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

void baoregon_emulator_init(void) {
    apple2_mem_reset();
    reset6502();
    boot_splash_init(&g_splash_state);
    boot_splash_button_edge_state_init(&g_edge_state);
    g_in_splash_menu = 1;
}

int baoregon_emulator_is_in_splash_menu(void) {
    return g_in_splash_menu;
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

    if (!g_in_splash_menu) {
        exec6502(BAOREGON_CYCLES_PER_FRAME);
    }

    /* Process audio state update */
    bunnie_audio_poll_and_apply(apple2_mem_get_audio_state());

    /* Render video frame based on current softswitch modes */
    int is_page2 = apple2_mem_is_page2_selected();
    int is_mixed = apple2_mem_is_mixed_mode();

    if (is_mixed) {
        bio_display_render_frame_mixed(is_page2, is_mixed, read6502, g_framebuffer);
    } else if (is_page2) {
        bio_display_render_frame_page(is_page2, read6502, g_framebuffer);
    } else {
        bio_display_render_frame(read6502, g_framebuffer);
    }

    return BAOREGON_CYCLES_PER_FRAME;
}
