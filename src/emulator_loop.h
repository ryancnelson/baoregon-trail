/*
 * emulator_loop.h -- Full frame-driven Apple II emulator loop interface.
 */
#ifndef EMULATOR_LOOP_H
#define EMULATOR_LOOP_H

#include <stdint.h>

/* Apple II standard timing: 1,023,000 cycles/sec -> ~17,050 cycles per 60Hz frame */
#define BAOREGON_CYCLES_PER_FRAME 17050u

void baoregon_emulator_init(void);
uint32_t baoregon_emulator_run_frame(void);
void baoregon_emulator_poll_input(void);
int baoregon_emulator_is_in_splash_menu(void);

#endif /* EMULATOR_LOOP_H */
