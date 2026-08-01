/*
 * emulator_loop.h -- Full frame-driven Apple II emulator loop interface.
 */
#ifndef EMULATOR_LOOP_H
#define EMULATOR_LOOP_H

#include <stdint.h>
#include <stddef.h>
#include "cartridge_layout.h"

/* Apple II standard timing: 1,023,000 cycles/sec -> ~17,050 cycles per 60Hz frame */
#define BAOREGON_CYCLES_PER_FRAME 17050u

void baoregon_emulator_init(void);
uint32_t baoregon_emulator_run_frame(void);
void baoregon_emulator_poll_input(void);
int baoregon_emulator_is_in_splash_menu(void);
void baoregon_emulator_reset_to_splash(void);
uint64_t baoregon_emulator_get_total_cycles(void);
int baoregon_emulator_is_audio_active(void);

/* Test/inspection hook: read-only access to the internal framebuffer
 * baoregon_emulator_run_frame() renders into. Not for hardware use (the
 * real BIO Core 0 DMA path reads it directly once the SPI DMA peripheral
 * exists -- see bio_display.h).
 *
 * IMPORTANT: this buffer is allocated at 320x240 (the eventual target
 * badge display resolution per README.md), but bio_display.h's render
 * functions do NOT scale -- they only ever write the native
 * BIO_DISPLAY_WIDTH x BIO_DISPLAY_HEIGHT (280x192) region in the
 * top-left corner. The remaining 320x240 - 280x192 = 23040 pixels
 * (right/bottom margins) are NEVER written by the renderer; they stay
 * whatever they were initialized to (static storage -> zero-initialized
 * at program start, but NOT necessarily zero after a real 6502 program
 * writes to $2000-$3FFF/$0400-$07FF -- that only affects the rendered
 * 280x192 region, never the margins). Scaling 280x192 up to fill the
 * full 320x240 (or switching to a 480x320 target) is explicitly
 * deferred until baochip confirms the target resolution -- see
 * bio_display.h's own "no scaling yet" note. Do not assume the margin
 * pixels are meaningful display content. */
const uint16_t *baoregon_emulator_get_framebuffer(void);

/* Safely copy internal framebuffer (320*240 uint16_t pixels) into dest.
 * Returns 0 on success, -1 if dest is NULL or dest_count < 320*240.
 *
 * Same margin caveat as baoregon_emulator_get_framebuffer(): only the
 * top-left 280x192 pixels of the copied 320x240 buffer are real
 * rendered content; the rest are unwritten margin pixels. */
int baoregon_emulator_copy_framebuffer(uint16_t *dest, size_t dest_count);

/* Read-only access to which cartridge slot is currently selected in the
 * boot-splash menu (or was last selected before the emulator left splash
 * mode and started running that game). Returns NULL only if state hasn't
 * been initialized yet (baoregon_emulator_init() not yet called) --
 * otherwise always returns a valid pointer into cartridge_layout.h's
 * cartridge_slots[] table, since boot_splash_current_slot() itself
 * clamps out-of-range indices back to slot 0 rather than ever returning
 * NULL for a valid state. Useful for a "now playing" HUD/debug overlay
 * or logging which game is loaded without exposing boot_splash.c's
 * internal state struct directly. */
const cartridge_slot_t *baoregon_emulator_get_current_slot(void);

#endif /* EMULATOR_LOOP_H */
