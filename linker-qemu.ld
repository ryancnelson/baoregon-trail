/*
 * linker-qemu.ld -- QEMU 'virt' machine linker script for baoregon-trail.
 *
 * Separate from the real Baochip-1x linker.ld (0x60000000 ReRAM XIP /
 * 0x61000000 SRAM, corrected 2026-08-06 -- see
 * docs/baochip-1x-memory-map-findings.md), which QEMU's generic 'virt'
 * machine doesn't model -- virt's RAM lives at 0x80000000 (confirmed via
 * `qemu-system-riscv32 -machine dumpdtb` + dtc, not assumed). This
 * collapses the real target's two-memory split into one contiguous RAM
 * region (virt only has one), but keeps the EXACT same symbol names
 * crt0.S expects (__data_start/__data_end/__data_load_start/
 * __bss_start/__bss_end/__stack_top) so the SAME crt0.S + C sources run
 * unmodified -- this is NOT a Baochip target, it's a "does our C code
 * correctly execute on a real RISC-V core" check ahead of real hardware
 * arriving.
 */

MEMORY
{
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 32M
}

STACK_SIZE = 16K;

ENTRY(_start)

SECTIONS
{
    .text :
    {
        KEEP(*(.text.entry))
        *(.text .text.*)
    } > RAM

    .rodata :
    {
        *(.rodata .rodata.*)
    } > RAM

    .dsk_images :
    {
        *(.dsk_images .dsk_images.*)
    } > RAM

    .data :
    {
        __data_start = .;
        *(.data .data.*)
        *(.sdata .sdata.* .sdata2 .sdata2.*)
        __data_end = .;
    } > RAM

    __data_load_start = LOADADDR(.data);
    __data_load_end = __data_load_start + SIZEOF(.data);

    .bss (NOLOAD) :
    {
        __bss_start = .;
        *(.bss .bss.*)
        *(.sbss .sbss.* .sbss2 .sbss2.*)
        *(COMMON)
        __bss_end = .;
    } > RAM

    .stack (NOLOAD) :
    {
        . = ALIGN(16);
        __stack_bottom = .;
        . += STACK_SIZE;
        __stack_top = .;
    } > RAM

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
    }
}
