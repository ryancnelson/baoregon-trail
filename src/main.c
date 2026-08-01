/*
 * main.c -- Main entry point for bare-metal RISC-V Baochip-1x execution.
 */
#include "emulator_loop.h"

int main(void) {
    baoregon_emulator_init();

    for (;;) {
        baoregon_emulator_run_frame();
    }

    return 0;
}
