/*
 * tests/test_disk2_controller_boot_rom.c -- RED-first test for
 * disk2_controller.c's Disk II boot PROM ($Cn00 slot ROM) exposure.
 *
 * Real Apple II hardware: each peripheral card in slot n exposes its
 * own 256-byte boot PROM at $Cn00-$CnFF (Disk II is conventionally slot
 * 6, i.e. $C600-$C6FF). The system ROM's own reset/autostart code JSRs
 * into $Cn00 for the lowest-numbered slot with a bootable card during
 * cold boot; that boot PROM code (embedded here byte-for-byte from the
 * real 341-0027-a.p5 chip, see roms/a2diskiing.zip) is what actually
 * drives the $C0E0-$C0EF softswitches to read track 0/sector 0 off the
 * disk and load DOS 3.3's own bootstrap into $0800, then jumps there.
 *
 * ($003E/$003F is DOS 3.3's own "current slot" zero-page vector,
 * populated by the boot PROM's own code as it runs -- NOT the initial
 * boot entry point itself, which is the system ROM's JSR $Cn00.)
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "../src/disk2_controller.h"

int main(void) {
    /* Byte 0 of the real 341-0027-a.p5 boot PROM: A2 20 (LDX #$20) --
     * confirmed byte-for-byte against the actual ROM file extracted
     * from roms/a2diskiing.zip earlier this session and against
     * apple2js's own BOOTSTRAP_ROM_16 constant (js/roms/cards/disk2.ts). */
    assert(disk2_controller_read_boot_rom(0x00) == 0xA2);
    assert(disk2_controller_read_boot_rom(0x01) == 0x20);
    assert(disk2_controller_read_boot_rom(0x02) == 0xA0);
    assert(disk2_controller_read_boot_rom(0x03) == 0x00);

    /* Last byte of the 256-byte PROM. */
    assert(disk2_controller_read_boot_rom(0xFF) == 0x00);

    /* A byte deep in the PROM's actual softswitch-driving code, picked
     * to catch a wrong-offset/off-by-one bug rather than just
     * re-checking the trivial first/last bytes -- verified against the
     * real 341-0027-a.p5 dump (roms/a2diskiing.zip) via `xxd`, not
     * guessed from disassembly. */
    assert(disk2_controller_read_boot_rom(0x30) == 0x8E);
    assert(disk2_controller_read_boot_rom(0x31) == 0xC0);

    printf("PASS: disk2_controller_read_boot_rom returns real 341-0027-a.p5 PROM bytes\n");
    printf("All tests passed.\n");
    return 0;
}
