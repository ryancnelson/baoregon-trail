/*
 * linker.ld -- Baochip-1x RISC-V linker script (Step 5, NEXT_STEPS.md).
 *
 * Two physical memories per PROJECT_GOALS.md Milestone 3 / BRAINSTORM.md /
 * README.md's block diagram:
 *
 *   ReRAM (XIP execution, non-volatile): 4.0 MiB @ 0x20000000
 *     - code executes directly in place (XIP) -- no copy-to-RAM step for
 *       .text, unlike a typical embedded target. This is why .text lives
 *       in ReRAM's LMA *and* VMA (see section placement below).
 *     - Per CLAUDE.md's current allocation: ~32 KiB bootloader/runtime,
 *       12 KiB Apple IIe Autostart ROM, 140 KiB OregonTrail.dsk, leaving
 *       ~3.8 MiB free for additional game .dsk images (BRAINSTORM.md
 *       section 5's cartridge offset table).
 *
 *   SRAM (volatile, fast): 2.0 MiB @ 0x40000000
 *     - .data's *runtime* copy, .bss, and the emulator's working state
 *       live here -- notably apple2_mem.c's 64 KB emulated Apple II RAM
 *       (g_ram[]), the Hi-Res/BIO framebuffers, and the C stack.
 *     - .data's *load* image (initial values) lives in ReRAM (LMA) and
 *       crt0 copies it to SRAM (VMA) at boot -- standard split
 *       LMA/VMA pattern for XIP targets with a separate writable RAM.
 *
 * Target ABI: rv32imac / ilp32 (riscv64-elf-gcc -march=rv32imac -mabi=ilp32,
 * confirmed working with Homebrew's riscv64-elf-gcc, which supports rv32
 * targets despite the package name).
 */

MEMORY
{
    /* 4.0 MiB ReRAM, XIP execution. Code + read-only data + embedded .dsk
     * game images all live here and are executed/read directly in place. */
    RERAM (rx) : ORIGIN = 0x20000000, LENGTH = 4M

    /* 2.0 MiB SRAM. Writable working state: apple2_mem.c's 64 KB emulated
     * RAM, BIO framebuffers, .data's runtime copy, .bss, and the stack. */
    SRAM  (rwx) : ORIGIN = 0x40000000, LENGTH = 2M
}

/* Stack grows down from the top of SRAM. Sized generously (16 KiB) for
 * the 6502 emulator core + soft-switch dispatch call depth -- revisit if
 * BIO coprocessor stacks need their own carve-out once bio_core*.c lands. */
STACK_SIZE = 16K;

ENTRY(_start)

SECTIONS
{
    /* --- ReRAM: code, read-only data, embedded disk images (XIP) --- */

    .text :
    {
        KEEP(*(.text.entry)) /* crt0's _start must lead .text */
        *(.text .text.*)
    } > RERAM

    .rodata :
    {
        *(.rodata .rodata.*)
    } > RERAM

    /* Embedded game .dsk byte arrays (tools/embed_disk.py output), per
     * BRAINSTORM.md section 5's ReRAM offset table. Kept in its own
     * output section (rather than folded into .rodata) so the multi-game
     * cartridge loader can reason about/relocate this region independently
     * as more games are added, and so `size`/`readelf` output makes disk-
     * image footprint visible at a glance during ReRAM budget planning. */
    .dsk_images :
    {
        *(.dsk_images .dsk_images.*)
    } > RERAM

    /* --- SRAM: initialized data (runtime copy), zeroed data, stack --- */

    /* .data's load address (LMA) stays in ReRAM; crt0 copies
     * [__data_load_start, __data_load_end) to __data_start in SRAM at
     * boot, matching the standard XIP-with-separate-RAM startup pattern. */
    .data : AT (LOADADDR(.dsk_images) + SIZEOF(.dsk_images))
    {
        __data_start = .;
        *(.data .data.*)
        __data_end = .;
    } > SRAM

    __data_load_start = LOADADDR(.data);
    __data_load_end = __data_load_start + SIZEOF(.data);

    .bss (NOLOAD) :
    {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    } > SRAM

    /* apple2_ram: the emulated Apple II's 64 KB address space
     * (apple2_mem.c's g_ram[]), documented as its own symbol range purely
     * so a memory map / `nm` inspection can immediately spot where the
     * emulated machine's RAM lives inside SRAM -- it is ordinary .bss
     * data as far as the linker is concerned, no special section needed
     * since apple2_mem.c already zero-initializes it via apple2_mem_reset()
     * rather than relying on static zero-init semantics alone. */

    .stack (NOLOAD) :
    {
        . = ALIGN(16);
        __stack_bottom = .;
        . += STACK_SIZE;
        __stack_top = .;
    } > SRAM

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
    }
}
