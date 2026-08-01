/*
 * main_qemu.c -- QEMU 'virt' target entry point. Boots directly into the
 * Oregon Trail title screen bitmap (embedded as a C byte array, see
 * oregon_trail_bitmap_data.h), bypassing the boot-splash menu -- this is
 * a verification harness ("does our C code run correctly as real RISC-V
 * machine code on a real emulated CPU core"), not the real Baochip-1x
 * bring-up target (that's main.c, unmodified, still boots into the real
 * splash-menu emulator loop for real hardware).
 */
#include "apple2_mem.h"
#include "cpu6502.h"
#include "oregon_trail_bitmap_data.h"

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

    for (;;) {
        __asm__ volatile("wfi");
    }

    return 0;
}
