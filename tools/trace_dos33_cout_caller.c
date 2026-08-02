/*
 * tools/trace_dos33_cout_caller.c -- Host utility to trace the exact caller PC,
 * stack state, and disassembly context of every JSR COUT ($FDED) call during
 * real DOS 3.3 Master boot.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "cpu6502.h"
#include "apple2_mem.h"
#include "disk2_controller.h"
#include "dos33_master_nib_disk_data.h"
#include "apple2e_system_rom.h"

#define SYSTEM_ROM_SIZE 16384
static uint8_t g_system_rom[SYSTEM_ROM_SIZE];

static void init_system_rom(void) {
    for (int i = 0; i < SYSTEM_ROM_SIZE; i++) {
        g_system_rom[i] = g_apple2e_system_rom[i];
    }
    /* Clean landing pads and RTI BRK handler */
    g_system_rom[0x2000] = 0x4C; g_system_rom[0x2001] = 0x00; g_system_rom[0x2002] = 0xE0; /* $E000: JMP $E000 */
    g_system_rom[0x2003] = 0x4C; g_system_rom[0x2004] = 0x00; g_system_rom[0x2005] = 0xE0; /* $E003: JMP $E000 */
    g_system_rom[0x2007] = 0x40;                                                           /* $E007: RTI */
    g_system_rom[0x3F58] = 0x60; /* IORST ($FF58) */
    g_system_rom[0x3E89] = 0x60; /* SETKBD ($FE89) */
    g_system_rom[0x3E93] = 0x60; /* SETVID ($FE93) */
    g_system_rom[0x3B2F] = 0x60; /* INIT ($FB2F) */
    g_system_rom[0x3CA8] = 0xA9; g_system_rom[0x3CA9] = 0x00; g_system_rom[0x3CAA] = 0x60; /* WAIT ($FCA8) */

    /* COUT patch at $FDED (offset $3DED) */
    static const uint8_t cout_code[] = {
        0x85, 0xFF,             /* STA $FF */
        0x98,                   /* TYA */
        0x48,                   /* PHA */
        0xA4, 0x24,             /* LDY $24 */
        0xA5, 0xFF,             /* LDA $FF */
        0x99, 0x00, 0x04,       /* STA $0400,Y */
        0xE6, 0x24,             /* INC $24 */
        0x68,                   /* PLA */
        0xA8,                   /* TAY */
        0xA5, 0xFF,             /* LDA $FF */
        0x60                    /* RTS */
    };
    for (size_t i = 0; i < sizeof(cout_code); i++) {
        g_system_rom[0x3DED + i] = cout_code[i];
    }
    g_system_rom[0x388E] = 0x4C; g_system_rom[0x388F] = 0xED; g_system_rom[0x3890] = 0xFD; /* JMP $FDED */
}

static disk2_nibble_track_t g_tracks[DISK2_MAX_TRACKS];

static void load_embedded_nib_disk(void) {
    for (int t = 0; t < G_DOS33_MASTER_TRACKS_NUM_TRACKS; t++) {
        for (int b = 0; b < G_DOS33_MASTER_TRACKS_MAX_TRACK_BYTES; b++) {
            g_tracks[t].data[b] = g_dos33_master_tracks_track_data[t][b];
        }
        g_tracks[t].length = g_dos33_master_tracks_track_lengths[t];
    }
}

int main(void) {
    init_system_rom();
    load_embedded_nib_disk();

    apple2_mem_reset();
    reset6502();
    apple2_mem_load_system_rom(g_system_rom);

    apple2_mem_set_disk_controller_mode(APPLE2_MEM_DISK_CONTROLLER_DISK2);
    disk2_controller_t *ctl = apple2_mem_get_disk2_controller();
    disk2_controller_load_nibble_disk(ctl, 0, g_tracks, 0);

    /* Boot ROM at $C600 with slot 6 calling convention (A=X=0x60) */
    pc = 0xC600;
    x = 0x60;
    a = 0x60;

    int cout_calls = 0;
    while (clockticks6502 < 30000000ULL) {
        uint16_t current_pc = pc;
        if (current_pc == 0xFDED || current_pc == 0xF88E) {
            cout_calls++;
            if (cout_calls == 1) {
                printf("=== FIRST COUT CALL (cycle %u) ===\n", clockticks6502);
                printf("Registers: PC=%04X A=%02X X=%02X Y=%02X SP=%02X Status=%02X\n",
                       pc, a, x, y, sp, status);
                printf("\nCaller memory around $1BA0-$1BD5:\n");
                for (uint16_t addr = 0x1BA0; addr <= 0x1BD5; addr += 16) {
                    printf("%04X: ", addr);
                    for (int i = 0; i < 16; i++) {
                        printf("%02X ", read6502(addr + i));
                    }
                    printf("\n");
                }
                printf("\nStack bytes (0x01E0-0x01FF):\n");
                uint8_t ptr_lo = read6502(0x40);
                uint8_t ptr_hi = read6502(0x41);
                uint16_t ptr = (ptr_hi << 8) | ptr_lo;
                printf("\nZero-page pointer $40/$41 = %04X\n", ptr);
                printf("Memory at %04X: ", ptr);
                for (int i = 0; i < 32; i++) {
                    printf("%02X ", read6502(ptr + i));
                }
                printf("\n");
                for (uint16_t addr = 0x01E0; addr <= 0x01FF; addr += 16) {
                    printf("%04X: ", addr);
                    for (int i = 0; i < 16; i++) {
                        printf("%02X ", read6502(addr + i));
                    }
                    printf("\n");
                }
                break;
            }
        }
        step6502();
    }
    return 0;
}
