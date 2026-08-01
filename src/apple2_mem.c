/*
 * apple2_mem.c -- 64KB Apple II memory map + $C000-$C0FF soft-switch
 * dispatch. Implements read6502()/write6502() per the bus contract in
 * cpu6502.h (locked 2026-07-31: baochip/Woz/Bunnie/Duke).
 *
 * See apple2_mem.h for the address-map / dispatch documentation.
 */
#include "apple2_mem.h"
#include "cpu6502.h"
#include "disk_trap.h"

#define APPLE2_RAM_SIZE 65536u
#define ROM_REGION_START 0xD000u

static uint8_t g_ram[APPLE2_RAM_SIZE];
static bunnie_audio_state_t g_audio_state;

/* Disk trap streaming cursor: tracks the next byte offset (0-255) that
 * $C0EC will return, auto-incrementing (wrapping mod 256) so software can
 * stream a whole 256-byte sector via repeated reads of one address. Reset
 * to 0 whenever a new sector is selected via $C0E0/$C0E1. */
static uint8_t g_disk_stream_cursor = 0;
static uint8_t g_pending_track = 0;

void apple2_mem_reset(void) {
    for (uint32_t i = 0; i < APPLE2_RAM_SIZE; i++) {
        g_ram[i] = 0;
    }
    bunnie_audio_init(&g_audio_state);
    g_disk_stream_cursor = 0;
    g_pending_track = 0;
}

void apple2_mem_set_disk_image(const uint8_t *image) {
    disk_trap_set_image(image);
}

bunnie_audio_state_t *apple2_mem_get_audio_state(void) {
    return &g_audio_state;
}

/* Shared soft-switch handling for both read and write -- the Apple II
 * toggles $C030 on ANY access, and the disk trap's data port ($C0EC) is
 * meant to be read, but write6502() still needs to recognize $C0E0/$C0E1
 * (write-only track/sector select) and ignore reads there gracefully. */
static void handle_soft_switch_write(uint16_t address, uint8_t value) {
    if (address == 0xC030) {
        bunnie_audio_trigger_toggle(&g_audio_state);
    } else if (address == 0xC0E0) {
        g_pending_track = value;
    } else if (address == 0xC0E1) {
        disk_trap_select_sector(g_pending_track, value);
        g_disk_stream_cursor = 0;
    }
    /* Any other $C0xx write: not yet implemented, silently ignored. */
}

static uint8_t handle_soft_switch_read(uint16_t address) {
    if (address == 0xC030) {
        bunnie_audio_trigger_toggle(&g_audio_state);
        return 0x00;
    }
    if (address == 0xC0EC) {
        uint8_t byte = disk_trap_read_byte(g_disk_stream_cursor);
        g_disk_stream_cursor = (uint8_t)(g_disk_stream_cursor + 1); /* wraps mod 256 */
        return byte;
    }
    /* Any other $C0xx read (including $C0E0/$C0E1, write-only): inert 0. */
    return 0x00;
}

uint8_t read6502(uint16_t address) {
    if (address >= 0xC000 && address <= 0xC0FF) {
        return handle_soft_switch_read(address);
    }
    return g_ram[address];
}

void write6502(uint16_t address, uint8_t value) {
    if (address >= 0xC000 && address <= 0xC0FF) {
        handle_soft_switch_write(address, value);
        return;
    }
    if (address >= ROM_REGION_START) {
        /* ROM region ($D000-$FFFF): write-protected. Real ROM image
         * loading is a later iteration (Step 5); for now writes here are
         * simply ignored so accidental/DOS-driven writes never corrupt
         * the boot ROM area. */
        return;
    }
    g_ram[address] = value;
}
