/*
 * tests/test_apple2_mem_disk2_boot_rom.c -- RED-first test proving
 * apple2_mem.c serves the Disk II boot PROM at $C600-$C6FF (slot 6)
 * when disk2 controller mode is active, exactly the mechanism real
 * Apple II boot code depends on (system ROM JSRs into $Cn00 for the
 * lowest bootable slot).
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"

int main(void) {
    apple2_mem_reset();

    /* Default mode (DISK_TRAP): $C600-$C6FF has no special meaning,
     * falls through to plain zeroed RAM -- matches existing behavior,
     * must NOT regress. */
    assert(read6502(0xC600) == 0x00);

    apple2_mem_set_disk_controller_mode(APPLE2_MEM_DISK_CONTROLLER_DISK2);

    /* DISK2 mode: $C600-$C6FF must serve the real boot PROM bytes. */
    assert(read6502(0xC600) == 0xA2);
    assert(read6502(0xC601) == 0x20);
    assert(read6502(0xC6FF) == 0x00);
    /* Deep byte, catches an off-by-one in the $C600 base offset. */
    assert(read6502(0xC630) == 0x8E);

    /* Writes to the boot PROM region must be silently ignored (it's
     * ROM, real hardware can't write to it), matching the existing
     * ROM write-protection convention used elsewhere in this file. */
    write6502(0xC600, 0xFF);
    assert(read6502(0xC600) == 0xA2);

    /* Switching back to DISK_TRAP mode restores the original
     * fall-through-to-RAM behavior at $C600-$C6FF. */
    apple2_mem_set_disk_controller_mode(APPLE2_MEM_DISK_CONTROLLER_DISK_TRAP);
    assert(read6502(0xC600) == 0x00);

    printf("PASS: apple2_mem.c serves the Disk II boot PROM at $C600-$C6FF in DISK2 mode\n");
    printf("All tests passed.\n");
    return 0;
}
