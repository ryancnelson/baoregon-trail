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

    /* 5. Unload system ROM (pass NULL) */
    apple2_mem_load_system_rom(NULL);
    assert(read6502(0xD000) == 0x00);

    printf("PASS: apple2_mem_load_system_rom verified\n");
    printf("All tests passed.\n");
    return 0;
}
