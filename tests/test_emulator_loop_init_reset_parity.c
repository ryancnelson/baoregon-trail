/*
 * RED test (behavioral, not just structural): baoregon_emulator_init()
 * and baoregon_emulator_reset_to_splash() are meant to be fully
 * equivalent "return to splash menu with fresh state" operations -- the
 * 3-button-combo hardware trigger calls init(), other callers use
 * reset_to_splash(). Twice now (audio active flag just landed; earlier
 * g_total_cycles at commit 0f538ac) these two functions have drifted
 * out of sync because they're maintained as separate copy-pasted
 * bodies with no single source of truth. This test doesn't fix the
 * duplication itself, but locks in EVERY observable piece of state
 * that must match after either reset path, from a running-game state,
 * so future drift is caught immediately rather than discovered one
 * field at a time.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../src/emulator_loop.h"
#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/video_apple2.h"

typedef struct {
    int in_splash_menu;
    uint64_t total_cycles;
    int audio_active;
    const cartridge_slot_t *current_slot;
    uint16_t framebuffer_pixel_0;
} observable_state_t;

static observable_state_t snapshot(void) {
    observable_state_t s;
    s.in_splash_menu = baoregon_emulator_is_in_splash_menu();
    s.total_cycles = baoregon_emulator_get_total_cycles();
    s.audio_active = baoregon_emulator_is_audio_active();
    s.current_slot = baoregon_emulator_get_current_slot();
    s.framebuffer_pixel_0 = baoregon_emulator_get_framebuffer()[0];
    return s;
}

static void run_a_game_session(void) {
    /* Enter game, render some Hi-Res content, run frames to accumulate
     * cycles -- put the emulator into a "definitely not fresh" state. */
    apple2_mem_set_button_state(2, 1);
    baoregon_emulator_poll_input();
    apple2_mem_set_button_state(2, 0);
    baoregon_emulator_poll_input();
    assert(baoregon_emulator_is_in_splash_menu() == 0);

    write6502(0xC057, 0x00); /* HIRES */
    write6502(HIRES_BASE_ADDR, 0x01);
    write6502(0xC030, 0x00); /* toggle speaker -> audio active */

    for (int i = 0; i < 3; i++) {
        baoregon_emulator_run_frame();
    }
}

static void assert_snapshots_equal(const observable_state_t *a, const observable_state_t *b, const char *label) {
    int ok = 1;
    if (a->in_splash_menu != b->in_splash_menu) {
        fprintf(stderr, "FAIL: %s: in_splash_menu differs (%d vs %d)\n", label, a->in_splash_menu, b->in_splash_menu);
        ok = 0;
    }
    if (a->total_cycles != b->total_cycles) {
        fprintf(stderr, "FAIL: %s: total_cycles differs (%llu vs %llu)\n", label,
                (unsigned long long)a->total_cycles, (unsigned long long)b->total_cycles);
        ok = 0;
    }
    if (a->audio_active != b->audio_active) {
        fprintf(stderr, "FAIL: %s: audio_active differs (%d vs %d)\n", label, a->audio_active, b->audio_active);
        ok = 0;
    }
    if (a->current_slot != b->current_slot) {
        fprintf(stderr, "FAIL: %s: current_slot pointer differs\n", label);
        ok = 0;
    }
    if (a->framebuffer_pixel_0 != b->framebuffer_pixel_0) {
        fprintf(stderr, "FAIL: %s: framebuffer_pixel_0 differs (0x%04X vs 0x%04X)\n", label,
                a->framebuffer_pixel_0, b->framebuffer_pixel_0);
        ok = 0;
    }
    if (!ok) {
        assert(0);
    }
    printf("PASS: %s\n", label);
}

static void test_init_and_reset_to_splash_produce_identical_observable_state(void) {
    /* Run the same "dirty" game session, then reset via init() and
     * separately via reset_to_splash(), and confirm every observable
     * getter reports the exact same values afterward. */
    baoregon_emulator_init();
    run_a_game_session();
    baoregon_emulator_init();
    observable_state_t after_init = snapshot();

    baoregon_emulator_init();
    run_a_game_session();
    baoregon_emulator_reset_to_splash();
    observable_state_t after_reset_to_splash = snapshot();

    assert_snapshots_equal(&after_init, &after_reset_to_splash,
        "test_init_and_reset_to_splash_produce_identical_observable_state");
}

int main(void) {
    test_init_and_reset_to_splash_produce_identical_observable_state();
    printf("All tests passed.\n");
    return 0;
}
