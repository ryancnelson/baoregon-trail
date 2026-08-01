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
#define LC_BANKED_REGION_START 0xD000u
#define LC_BANKED_REGION_END 0xDFFFu
#define LC_BANKED_REGION_SIZE 4096u

static uint8_t g_ram[APPLE2_RAM_SIZE];
static bunnie_audio_state_t g_audio_state;

/* Language Card ($C080-$C08F) state. Only $D000-$DFFF is bank-switched on
 * real Apple II hardware -- $E000-$FFFF is a single shared RAM region
 * regardless of bank selection, so it needs no separate backing store
 * (g_ram[] already covers it). g_lc_bank1/g_lc_bank2 hold the two
 * independent 4KB RAM images that can be swapped into $D000-$DFFF. */
static uint8_t g_lc_bank1[LC_BANKED_REGION_SIZE];
static uint8_t g_lc_bank2[LC_BANKED_REGION_SIZE];
static int g_lc_read_ram = 0;   /* 0 = read ROM, 1 = read RAM */
static int g_lc_write_enable = 0; /* 0 = write-protected, 1 = write-enabled */
static int g_lc_bank1_selected = 0; /* 0 = bank 2, 1 = bank 1 */

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
    for (uint32_t i = 0; i < LC_BANKED_REGION_SIZE; i++) {
        g_lc_bank1[i] = 0;
        g_lc_bank2[i] = 0;
    }
    g_lc_read_ram = 0;
    g_lc_write_enable = 0;
    g_lc_bank1_selected = 0;
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

/* Apply the Language Card softswitch selected by a $C080-$C08F access.
 * Per the canonical Apple II LC table (bank 2 shown; add $08 for bank 1,
 * which mirrors the same read/write semantics for its own independent
 * $D000-$DFFF bank):
 *   $C080/$C084 = read RAM,  write protect
 *   $C081/$C085 = read ROM,  write enable
 *   $C082/$C086 = read ROM,  write protect
 *   $C083/$C087 = read RAM,  write enable
 * Triggered on ANY access (read or write), same "any access" rule as
 * Bunnie's $C030 speaker trap. The double-read-to-latch-write-enable
 * quirk of real hardware is simplified away here: a single access with
 * the write-enable bit pattern takes effect immediately (documented
 * simplification, matches other soft-switch simplifications in this
 * file per BRAINSTORM.md section 4's precedent for Disk II timing). */
static void apply_language_card_switch(uint16_t address) {
    uint8_t low_nibble = (uint8_t)(address & 0x07);
    g_lc_bank1_selected = (address & 0x08) != 0;
    switch (low_nibble) {
        case 0x00: /* $C080/$C088: read RAM, write protect */
            g_lc_read_ram = 1;
            g_lc_write_enable = 0;
            break;
        case 0x01: /* $C081/$C089: read ROM, write enable */
            g_lc_read_ram = 0;
            g_lc_write_enable = 1;
            break;
        case 0x02: /* $C082/$C08A: read ROM, write protect */
            g_lc_read_ram = 0;
            g_lc_write_enable = 0;
            break;
        case 0x03: /* $C083/$C08B: read RAM, write enable */
            g_lc_read_ram = 1;
            g_lc_write_enable = 1;
            break;
        default: /* $C084-$C087/$C08C-$C08F mirror $C080-$C083 */
            switch ((uint8_t)(low_nibble & 0x03)) {
                case 0x00: g_lc_read_ram = 1; g_lc_write_enable = 0; break;
                case 0x01: g_lc_read_ram = 0; g_lc_write_enable = 1; break;
                case 0x02: g_lc_read_ram = 0; g_lc_write_enable = 0; break;
                case 0x03: g_lc_read_ram = 1; g_lc_write_enable = 1; break;
            }
            break;
    }
}

static uint8_t *current_lc_bank(void) {
    return g_lc_bank1_selected ? g_lc_bank1 : g_lc_bank2;
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
    } else if (address >= 0xC080 && address <= 0xC08F) {
        apply_language_card_switch(address);
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
    if (address >= 0xC080 && address <= 0xC08F) {
        apply_language_card_switch(address);
        return 0x00;
    }
    /* Any other $C0xx read (including $C0E0/$C0E1, write-only): inert 0. */
    return 0x00;
}

uint8_t read6502(uint16_t address) {
    if (address >= 0xC000 && address <= 0xC0FF) {
        return handle_soft_switch_read(address);
    }
    if (address >= LC_BANKED_REGION_START && g_lc_read_ram) {
        /* $D000-$DFFF is banked (two independent 4KB images); $E000-$FFFF
         * is shared RAM but still gated by the same read-RAM/read-ROM
         * softswitch state as the banked region -- real Apple II LC
         * hardware controls read/write access to the whole $D000-$FFFF
         * space with one switch, only the bank SELECT is $D000-$DFFF-only. */
        if (address <= LC_BANKED_REGION_END) {
            return current_lc_bank()[address - LC_BANKED_REGION_START];
        }
        return g_ram[address];
    }
    return g_ram[address];
}

void write6502(uint16_t address, uint8_t value) {
    if (address >= 0xC000 && address <= 0xC0FF) {
        handle_soft_switch_write(address, value);
        return;
    }
    if (address >= LC_BANKED_REGION_START) {
        if (!g_lc_write_enable) {
            /* write-protected: silently ignored, matches real Apple II
             * LC/ROM write-protection semantics. */
            return;
        }
        if (address <= LC_BANKED_REGION_END) {
            current_lc_bank()[address - LC_BANKED_REGION_START] = value;
        } else {
            g_ram[address] = value;
        }
        return;
    }
    g_ram[address] = value;
}

