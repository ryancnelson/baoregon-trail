/*
 * tests/test_dos33_composed_boot.c -- End-to-End Composed System Disk Boot Integration Test.
 *
 * Validates the composed system (cpu6502 + apple2_mem + disk_trap + embedded .dsk image):
 * 1. Loads embedded DOS 3.3 boot disk image (g_dos33_sample_dsk) via disk_trap_load_image.
 * 2. Simulates ROM bootloader reading sector (Track 0, Sector 0) into $0800.
 * 3. Sets PC=$0800 and executes 6502 bootloader code.
 * 4. Verifies "HELLO WORLD" in Apple II high-ASCII rendered to text RAM ($0400-$040A)
 *    and TEXT mode softswitch ($C051) selected.
 */
#include <assert.h>
#include <stdio.h>
#include "apple2_mem.h"
#include "cpu6502.h"
#include "disk_trap.h"
#include "embedded_disk_dos33.h"

static void test_dos33_composed_system_boot(void) {
    apple2_mem_reset();
    reset6502();

    /* 1. Load embedded .dsk image into disk trap */
    disk_trap_set_image(g_dos33_sample_dsk);

    /* 2. Simulate Apple II ROM bootloader reading Track 0 Sector 0 into $0800 */
    write6502(0xC0E0, 0x00); /* Track 0 */
    write6502(0xC0E1, 0x00); /* Sector 0 */

    for (int i = 0; i < 256; i++) {
        uint8_t b = read6502(0xC0EC); /* read from disk trap data port */
        write6502(0x0800 + i, b);
    }

    /* Verify sector 0 opcode bytes were loaded into $0800 */
    assert(read6502(0x0800) == 0xA2); /* LDX #$00 */
    assert(read6502(0x0801) == 0x00);

    /* 3. Execute 6502 bootloader at $0800 */
    pc = 0x0800;
    exec6502(300); /* execute 300 cycles for 11 chars + softswitches */

    /* 4. Verify text RAM $0400 contains "HELLO WORLD" in high ASCII */
    assert(read6502(0x0400) == ('H' | 0x80));
    assert(read6502(0x0401) == ('E' | 0x80));
    assert(read6502(0x0402) == ('L' | 0x80));
    assert(read6502(0x0403) == ('L' | 0x80));
    assert(read6502(0x0404) == ('O' | 0x80));
    assert(read6502(0x0405) == (' ' | 0x80));
    assert(read6502(0x0406) == ('W' | 0x80));
    assert(read6502(0x0407) == ('O' | 0x80));
    assert(read6502(0x0408) == ('R' | 0x80));
    assert(read6502(0x0409) == ('L' | 0x80));
    assert(read6502(0x040A) == ('D' | 0x80));

    /* Verify softswitches switched to TEXT mode ($C051) & PAGE1 ($C054) */
    assert(apple2_mem_is_text_mode() == 1);
    assert(apple2_mem_is_page2_selected() == 0);

    printf("PASS: test_dos33_composed_system_boot\n");
}

int main(void) {
    test_dos33_composed_system_boot();
    return 0;
}
