/*
 * crt0.S -- minimal RISC-V startup code for the Baochip-1x bring-up target.
 *
 * Responsibilities (standard XIP-with-separate-RAM startup sequence):
 *   1. Set the stack pointer to __stack_top (linker.ld, SRAM).
 *   2. Copy .data's initialized values from ReRAM (LMA) to SRAM (VMA).
 *   3. Zero .bss.
 *   4. Call main(). If main() ever returns, spin forever (WFI loop) --
 *      there is no OS to return to.
 *
 * Placed in .text.entry so linker.ld's KEEP(*(.text.entry)) pins _start
 * as the first thing in ReRAM's .text, matching ENTRY(_start).
 */
    .section .text.entry
    .global _start
    .type _start, @function

_start:
    /* 1. Stack pointer. */
    la      sp, __stack_top

    /* 2. Copy .data: ReRAM (LMA, __data_load_start) -> SRAM (VMA, __data_start). */
    la      t0, __data_load_start
    la      t1, __data_start
    la      t2, __data_end
copy_data_loop:
    bge     t1, t2, copy_data_done
    lw      t3, 0(t0)
    sw      t3, 0(t1)
    addi    t0, t0, 4
    addi    t1, t1, 4
    j       copy_data_loop
copy_data_done:

    /* 3. Zero .bss. */
    la      t0, __bss_start
    la      t1, __bss_end
zero_bss_loop:
    bge     t0, t1, zero_bss_done
    sw      zero, 0(t0)
    addi    t0, t0, 4
    j       zero_bss_loop
zero_bss_done:

    /* 4. Jump to main(). Never returns under normal operation. */
    call    main

halt:
    wfi
    j       halt

    .size _start, . - _start
