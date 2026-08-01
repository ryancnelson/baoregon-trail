/*
 * tests/test_apple2_mem_load_system_rom.c -- unit test for apple2_mem_load_system_rom API.
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"

int main(void) {
    apple2_mem_reset();

    /* 1. Default state (no ROM loaded): reads at $D000 return g_ram[0xD000] (0x00) */
    write6502(0xD000, 0x00);
    assert(read6502(0xD000) == 0x00);

    /* 2. Load 16KB mock system ROM */
    static uint8_t mock_rom[16384];
    memset(mock_rom, 0x55, sizeof(mock_rom));
    mock_rom[0x1000] = 0xAA; /* $D000 offset is 0xD000 - 0xC000 = 0x1000 */

    apple2_mem_load_system_rom(mock_rom);

    /* Post-reset state is ROM read (g_lc_read_ram == 0), so read6502(0xD000) returns 0xAA */
    assert(read6502(0xD000) == 0xAA);

    /* 3. Flip LC softswitch to read RAM ($C080) */
    (void)read6502(0xC080);
    /* Now reads at $D000 should come from RAM (0x00), not ROM */
    assert(read6502(0xD000) == 0x00);

    /* 4. Flip LC softswitch back to read ROM ($C081) */
    (void)read6502(0xC081);
    assert(read6502(0xD000) == 0xAA);

    /* 6. apple2_mem_reset() must NOT clear the loaded system ROM pointer --
     * real Apple II ROM chips don't get erased by a soft reset (or the
     * 3-button reset combo). Re-select ROM read after reset (default LC
     * state already selects ROM read post-reset, so no $C081 needed) and
     * confirm the same ROM bytes are still served. */
    apple2_mem_reset();
    assert(read6502(0xD000) == 0xAA);

    /* 7. Write-protection must still apply when a system ROM is loaded --
     * the write path is governed purely by the LC write-enable
     * softswitch, unrelated to whether a ROM is loaded for reads. A
     * write to $D000 while write-protected (the post-reset default)
     * must be silently ignored, not accidentally "succeed" into
     * g_ram[] behind the ROM's back (which read6502() would never show
     * anyway since ROM takes priority when g_lc_read_ram==0, but the
     * write itself must still be correctly blocked, not just invisible
     * by coincidence). */
    (void)read6502(0xC080); /* select RAM read+write-protect at $D000-$DFFF */
    write6502(0xD000, 0x99); /* should be ignored: write-protected */
    (void)read6502(0xC081); /* re-select ROM read for the actual assertion */
    assert(read6502(0xD000) == 0xAA); /* still ROM's value, write never took effect */

    /* 8. Unload system ROM (pass NULL) */
    apple2_mem_load_system_rom(NULL);
    assert(read6502(0xD000) == 0x00);

    /* 9. $C100-$CFFF (I/O firmware + expansion ROM region) must ALSO be
     * served from the loaded system ROM image when present -- real Apple
     * IIe hardware's peripheral-card ROM space lives here, and boot code
     * (Disk II boot PROM, DOS 3.3's own bootstrap) can JMP into this
     * range as part of a real boot sequence. Previously only $D000-$FFFF
     * (gated by the LC softswitch) was wired up; $C100-$CFFF fell
     * through to the always-zeroed g_ram[] fallback even with a real ROM
     * loaded, silently serving garbage/zero bytes instead of the actual
     * firmware -- exactly the "JMP-into-peripheral-ROM boundary" gap
     * documented in tools/boot_with_real_rom.c's file comment. This
     * range is NOT gated by the LC softswitch on real hardware (that
     * only controls $D000-$FFFF); it's always-ROM whenever a system ROM
     * image is loaded, matching how $C100-$CFFF has no bank-switching at
     * all on the Apple IIe. */
    static uint8_t mock_rom2[16384];
    memset(mock_rom2, 0x00, sizeof(mock_rom2));
    mock_rom2[0x0100] = 0x77; /* $C100 offset is $C100 - $C000 = $0100 */
    mock_rom2[0x0FFF] = 0x88; /* $CFFF offset is $CFFF - $C000 = $0FFF */
    apple2_mem_load_system_rom(mock_rom2);

    assert(read6502(0xC100) == 0x77);
    assert(read6502(0xCFFF) == 0x88);

    apple2_mem_load_system_rom(NULL);
    assert(read6502(0xC100) == 0x00);

    /* 10. Writes to $C100-$CFFF must still go to g_ram[] normally --
     * write6502() was intentionally NOT touched by the $C100-$CFFF read
     * fix above (that region isn't write-protected the way $D000-$FFFF
     * is; real Apple IIe peripheral-card slot space has no such
     * protection). Confirms the read fix didn't accidentally also
     * block/redirect writes: with no ROM loaded, a write followed by a
     * read round-trips normally through g_ram[]. */
    write6502(0xC100, 0x42);
    assert(read6502(0xC100) == 0x42);

    /* 11. With a ROM loaded, a write to $C100-$CFFF still succeeds into
     * g_ram[] (write6502() has no ROM-awareness at all for this range),
     * but the SUBSEQUENT read is masked by ROM priority -- the write
     * "landed" in RAM underneath, it's just not visible via read6502()
     * while a ROM is loaded, exactly mirroring how $D000-$FFFF's
     * ROM-vs-RAM priority already works. Confirms this masking is
     * consistent between the two ROM-backed ranges, not accidentally
     * different (e.g. one leaking the RAM write through, the other
     * not). */
    apple2_mem_load_system_rom(mock_rom2);
    write6502(0xC100, 0x33); /* succeeds into g_ram[], but masked by ROM */
    assert(read6502(0xC100) == 0x77); /* still ROM's value, not the write */
    apple2_mem_load_system_rom(NULL);
    assert(read6502(0xC100) == 0x33); /* the earlier write is visible once ROM unloads */

    printf("PASS: apple2_mem_load_system_rom verified\n");
    printf("All tests passed.\n");
    return 0;
}
