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

/* Display-mode softswitch state ($C050-$C057). Post-reset defaults match
 * real Apple II hardware: TEXT mode, PAGE1, LORES, full-screen (not
 * MIXED). */
static int g_display_text_mode = 1;
static int g_display_mixed_mode = 0;
static int g_display_page2_selected = 0;
static int g_display_hires_mode = 0;

/* Keyboard input latch ($C000) + strobe ($C010). g_keyboard_ascii holds
 * the last injected key's ASCII value (7-bit); g_keyboard_strobe_pending
 * tracks whether the high bit (0x80, "strobe") should be reported on the
 * next $C000 read -- cleared by ANY access to $C010, matching real
 * hardware's "any access clears strobe" semantics (same rule as the
 * other soft-switches in this file). */
static uint8_t g_keyboard_ascii = 0;
static int g_keyboard_strobe_pending = 0;

/* Pushbutton/paddle inputs ($C061-$C063). Index 0/1/2 maps to PB0/PB1/PB2. */
static int g_button_pressed[3] = {0, 0, 0};

/* Annunciator outputs AN0-AN3 ($C058-$C05F). Defaults to off, matching
 * real Apple II post-reset state. */
static int g_annunciator_on[4] = {0, 0, 0, 0};

/* Paddle analog timer state ($C064/$C065 PADDLE0/PADDLE1, $C070 PDRIVE).
 * g_paddle_countdown_target[n] is the read-count set via
 * apple2_mem_set_paddle_value() -- how many PADDLEn reads the RC
 * countdown takes to expire after $C070 arms it. g_paddle_reads_remaining[n]
 * is the live countdown, decremented on each PADDLEn read while armed;
 * g_paddle_armed[n] tracks whether that paddle's countdown is currently
 * running (both paddles are armed together by one $C070 access, matching
 * real hardware where PDRIVE triggers both RC circuits at once). */
static uint8_t g_paddle_countdown_target[2] = {0, 0};
static int g_paddle_reads_remaining[2] = {0, 0};
static int g_paddle_armed[2] = {0, 0};

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
    g_display_text_mode = 1;
    g_display_mixed_mode = 0;
    g_display_page2_selected = 0;
    g_display_hires_mode = 0;
    g_keyboard_ascii = 0;
    g_keyboard_strobe_pending = 0;
    g_button_pressed[0] = 0;
    g_button_pressed[1] = 0;
    g_button_pressed[2] = 0;
    g_annunciator_on[0] = 0;
    g_annunciator_on[1] = 0;
    g_annunciator_on[2] = 0;
    g_annunciator_on[3] = 0;
    g_paddle_countdown_target[0] = 0;
    g_paddle_countdown_target[1] = 0;
    g_paddle_reads_remaining[0] = 0;
    g_paddle_reads_remaining[1] = 0;
    g_paddle_armed[0] = 0;
    g_paddle_armed[1] = 0;
}

void apple2_mem_set_disk_image(const uint8_t *image) {
    disk_trap_set_image(image);
}

bunnie_audio_state_t *apple2_mem_get_audio_state(void) {
    return &g_audio_state;
}

int apple2_mem_is_text_mode(void) {
    return g_display_text_mode;
}

int apple2_mem_is_mixed_mode(void) {
    return g_display_mixed_mode;
}

int apple2_mem_is_page2_selected(void) {
    return g_display_page2_selected;
}

int apple2_mem_is_hires_mode(void) {
    return g_display_hires_mode;
}

void apple2_mem_inject_key(uint8_t ascii_value) {
    g_keyboard_ascii = (uint8_t)(ascii_value & 0x7F);
    g_keyboard_strobe_pending = 1;
}

void apple2_mem_set_button_state(int button_index, int pressed) {
    if (button_index < 0 || button_index > 2) {
        return; /* out of range: not one of PB0/PB1/PB2, silently ignored */
    }
    g_button_pressed[button_index] = pressed ? 1 : 0;
}

int apple2_mem_get_annunciator_state(int annunciator_index) {
    if (annunciator_index < 0 || annunciator_index > 3) {
        return 0; /* out of range: not one of AN0-AN3 */
    }
    return g_annunciator_on[annunciator_index];
}

void apple2_mem_set_paddle_value(int paddle_index, uint8_t value) {
    if (paddle_index < 0 || paddle_index > 1) {
        return; /* out of range: not PADDLE0/PADDLE1, silently ignored */
    }
    g_paddle_countdown_target[paddle_index] = value;
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

/* Apply the display-mode softswitch selected by a $C050-$C057 access.
 * Standard Apple II decode: each even/odd pair is one on/off switch,
 * triggered by ANY access (read or write), same rule as the other
 * soft-switches in this file:
 *   $C050 = GRAPHICS   $C051 = TEXT
 *   $C052 = FULL       $C053 = MIXED
 *   $C054 = PAGE1      $C055 = PAGE2
 *   $C056 = LORES      $C057 = HIRES
 * The four pairs are independent -- setting one never touches the
 * others. */
static void apply_display_mode_switch(uint16_t address) {
    switch (address) {
        case 0xC050: g_display_text_mode = 0; break;
        case 0xC051: g_display_text_mode = 1; break;
        case 0xC052: g_display_mixed_mode = 0; break;
        case 0xC053: g_display_mixed_mode = 1; break;
        case 0xC054: g_display_page2_selected = 0; break;
        case 0xC055: g_display_page2_selected = 1; break;
        case 0xC056: g_display_hires_mode = 0; break;
        case 0xC057: g_display_hires_mode = 1; break;
        default: break;
    }
}

/* Apply the annunciator softswitch selected by a $C058-$C05F access.
 * Same independent on/off pair pattern as the display-mode switches:
 *   $C058/$C059 = AN0 off/on   $C05A/$C05B = AN1 off/on
 *   $C05C/$C05D = AN2 off/on   $C05E/$C05F = AN3 off/on */
static void apply_annunciator_switch(uint16_t address) {
    int annunciator_index = (int)((address - 0xC058) / 2);
    int turn_on = (address & 0x01) != 0;
    if (annunciator_index >= 0 && annunciator_index <= 3) {
        g_annunciator_on[annunciator_index] = turn_on;
    }
}

/* Arm the paddle RC countdown for both PADDLE0 and PADDLE1 -- real
 * hardware's PDRIVE trigger charges both RC circuits with one access. */
static void trigger_paddle_drive(void) {
    g_paddle_armed[0] = 1;
    g_paddle_reads_remaining[0] = g_paddle_countdown_target[0];
    g_paddle_armed[1] = 1;
    g_paddle_reads_remaining[1] = g_paddle_countdown_target[1];
}

/* Read PADDLEn: report the strobe bit (0x80) while armed and the
 * countdown hasn't yet reached zero, decrementing on each read. Once
 * reads_remaining hits zero the countdown has "discharged" and stays
 * disarmed until the next $C070 trigger. */
static uint8_t read_paddle(int paddle_index) {
    if (!g_paddle_armed[paddle_index]) {
        return 0x00;
    }
    if (g_paddle_reads_remaining[paddle_index] <= 0) {
        g_paddle_armed[paddle_index] = 0;
        return 0x00;
    }
    g_paddle_reads_remaining[paddle_index]--;
    return 0x80;
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
    } else if (address == 0xC010) {
        g_keyboard_strobe_pending = 0;
    } else if (address >= 0xC050 && address <= 0xC057) {
        apply_display_mode_switch(address);
    } else if (address >= 0xC058 && address <= 0xC05F) {
        apply_annunciator_switch(address);
    } else if (address == 0xC070) {
        trigger_paddle_drive();
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
    if (address == 0xC000) {
        uint8_t result = g_keyboard_ascii;
        if (g_keyboard_strobe_pending) {
            result |= 0x80;
        }
        return result;
    }
    if (address == 0xC010) {
        g_keyboard_strobe_pending = 0;
        return 0x00;
    }
    if (address >= 0xC050 && address <= 0xC057) {
        apply_display_mode_switch(address);
        return 0x00;
    }
    if (address >= 0xC058 && address <= 0xC05F) {
        apply_annunciator_switch(address);
        return 0x00;
    }
    if (address == 0xC061) {
        return g_button_pressed[0] ? 0x80 : 0x00;
    }
    if (address == 0xC062) {
        return g_button_pressed[1] ? 0x80 : 0x00;
    }
    if (address == 0xC063) {
        return g_button_pressed[2] ? 0x80 : 0x00;
    }
    if (address == 0xC064) {
        return read_paddle(0);
    }
    if (address == 0xC065) {
        return read_paddle(1);
    }
    if (address == 0xC070) {
        trigger_paddle_drive();
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

