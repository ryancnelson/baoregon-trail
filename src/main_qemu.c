/*
 * main_qemu.c -- QEMU 'virt' target entry point. Boots directly into the
 * Oregon Trail title screen bitmap (embedded as a C byte array, see
 * oregon_trail_bitmap_data.h), bypassing the boot-splash menu -- this is
 * a verification harness ("does our C code run correctly as real RISC-V
 * machine code on a real emulated CPU core"), not the real Baochip-1x
 * bring-up target (that's main.c, unmodified, still boots into the real
 * splash-menu emulator loop for real hardware).
 *
 * NEXT_STEPS.md Step 6 (QEMU ramfb Live Display, dev-only): now also
 * registers a ramfb device (see tools/ramfb_display.c) and continuously
 * re-renders + re-pushes the current framebuffer every loop iteration,
 * so `qemu-system-riscv32 -M virt -device ramfb -display cocoa` shows a
 * live, continuously-updating window instead of requiring a one-shot
 * memory dump + host-side render step after the guest halts.
 *
 * ramfb_display_init() gracefully no-ops (returns 0) if QEMU was launched
 * without -device ramfb -- this file still runs fine headless in that
 * case (matches every other QEMU test harness in this repo, e.g.
 * tests/test_bio_audio_qemu.S's -nographic invocation), it just skips
 * the live-display push.
 */
#include "apple2_mem.h"
#include "cpu6502.h"
#include "bio_display.h"
#include "oregon_trail_bitmap_data.h"

/* From tools/ramfb_display.c -- not declared in a shared header since
 * this is a QEMU-dev-only consumer, same pattern as
 * fb_terminal_viewer_print()'s forward declaration in the other demo
 * runners (tools/hires_demo_runner.c etc). */
int ramfb_display_init(void);
void ramfb_display_update(const uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]);

static uint16_t g_framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];

int main(void) {
    apple2_mem_reset();

    for (int i = 0; i < 8192; i++) {
        write6502((uint16_t)(0x2000 + i), oregon_trail_bitmap[i]);
    }

    /* Softswitch addresses verified against apple2_mem.c's real dispatch
     * (same values used by tools/oregon_trail_title.s/checkerboard.s). */
    write6502(0xC057, 0x00); /* HIRES on */
    write6502(0xC052, 0x00); /* MIXED off (full-screen) */
    write6502(0xC050, 0x00); /* GRAPHICS on (TEXT off) */
    write6502(0xC054, 0x00); /* PAGE2 off (page 1) */

    int have_ramfb = ramfb_display_init();

    for (;;) {
        if (have_ramfb) {
            /* Continuous refresh loop (Step 6's last checklist item):
             * re-decode the current Hi-Res buffer and re-push every
             * iteration -- a real busy-spin, NOT `wfi` (which would halt
             * the vCPU waiting for an interrupt that never arrives here,
             * since no timer/PLIC interrupt source is wired up in this
             * harness -- that would render exactly once then freeze,
             * which is NOT what "continuous" means). The static bitmap
             * never changes in this harness, but this loop shape is
             * what makes a live 6502 program (once one runs here instead
             * of a static demo) visibly update frame-to-frame -- the
             * render+push call sites don't change when real 6502
             * execution replaces the one-shot bitmap load above. */
            bio_display_render_frame_auto_text_aware(
                apple2_mem_is_hires_mode(), apple2_mem_is_page2_selected(),
                apple2_mem_is_mixed_mode(), apple2_mem_is_text_mode(),
                read6502, g_framebuffer);
            ramfb_display_update(g_framebuffer);
        } else {
            /* No -device ramfb on the QEMU command line -- nothing to
             * push to, so idle instead of burning host CPU in a tight
             * spin loop for no visible benefit. */
            __asm__ volatile("wfi");
        }
    }

    return 0;
}
