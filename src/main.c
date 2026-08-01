/*
 * main.c -- minimal RISC-V bring-up scaffold (Step 5, NEXT_STEPS.md).
 *
 * This is NOT the real emulator main loop -- it exists solely to give
 * linker.ld / crt0.S / the cross-compile Makefile.riscv something real to
 * link and verify end-to-end (an .elf that actually places .text in ReRAM,
 * .data/.bss in SRAM, and boots to a reset 6502 without a linker or
 * startup-code bug). The real main loop (exec6502() driving loop, BIO core
 * kickoff, badge input polling, etc.) is a follow-up iteration once
 * baochip/Woz confirm the Dabao SDK hardware-init sequence.
 *
 * Deliberately exercises one static buffer (g_bringup_marker) so .data's
 * ReRAM->SRAM copy in crt0.S has something non-zero to prove it actually
 * ran, not just .bss zeroing.
 */
#include "apple2_mem.h"
#include "cpu6502.h"

/* Non-zero initialized data: proves crt0.S's .data copy (ReRAM LMA ->
 * SRAM VMA) actually executed, not just relying on .bss's zero-init. */
static volatile uint32_t g_bringup_marker = 0xDEADBEEFu;

int main(void) {
    apple2_mem_reset();
    reset6502();

    /* Touch the marker so it's not optimized away and so a debugger/JTAG
     * inspection of SRAM can confirm the .data copy landed correctly. */
    while (g_bringup_marker != 0) {
        g_bringup_marker = g_bringup_marker; /* no-op, keeps it live */
        break;
    }

    for (;;) {
        /* Bring-up scaffold: no game loop yet. Real exec6502() driving
         * loop + BIO core kickoff is a follow-up iteration. */
    }

    return 0;
}
