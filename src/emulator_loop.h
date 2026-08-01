/*
 * emulator_loop.h -- Full frame-driven Apple II emulator loop interface.
 */
#ifndef EMULATOR_LOOP_H
#define EMULATOR_LOOP_H

#include <stdint.h>
#include <stddef.h>

/* Apple II standard timing: 1,023,000 cycles/sec -> ~17,050 cycles per 60Hz frame */
#define BAOREGON_CYCLES_PER_FRAME 17050u

void baoregon_emulator_init(void);
uint32_t baoregon_emulator_run_frame(void);
void baoregon_emulator_poll_input(void);
int baoregon_emulator_is_in_splash_menu(void);

/* Test/inspection hook: read-only access to the internal framebuffer
 * baoregon_emulator_run_frame() renders into. Not for hardware use (the
 * real BIO Core 0 DMA path reads it directly once the SPI DMA peripheral
 * exists -- see bio_display.h). */
const uint16_t *baoregon_emulator_get_framebuffer(void);

/* Safely copy internal framebuffer (320*240 uint16_t pixels) into dest.
 * Returns 0 on success, -1 if dest is NULL or dest_count < 320*240. */
int baoregon_emulator_copy_framebuffer(uint16_t *dest, size_t dest_count);

#endif /* EMULATOR_LOOP_H */
