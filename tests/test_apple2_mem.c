/*
 * RED test: apple2_mem.c's real read6502/write6502 wiring -- 64KB RAM
 * backing, ROM write-protection, and the $C000-$C0FF soft-switch dispatch
 * to Duke's disk trap ($C0E0/$C0E1/$C0EC) and Bunnie's audio trap ($C030).
 *
 * Woz's cpu6502.c has landed and is GREEN on main -- this is the first
 * real (non-mock) exercise of apple2_mem.c against the locked
 * read6502(uint16_t)/write6502(uint16_t, uint8_t) signatures.
 */
#include <stdio.h>
#include <string.h>
#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/disk_sector_layout.h"

static int failures = 0;

#define CHECK(cond, label) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", label); \
            failures++; \
        } else { \
            printf("PASS: %s\n", label); \
        } \
    } while (0)

static uint8_t g_mock_disk_image[DOS33_DISK_IMAGE_SIZE];

static void fill_mock_disk_image(void) {
    for (uint32_t i = 0; i < DOS33_DISK_IMAGE_SIZE; i++) {
        g_mock_disk_image[i] = (uint8_t)(i & 0xFF);
    }
}

static void test_plain_ram_read_write_roundtrip(void) {
    apple2_mem_reset();

    write6502(0x0300, 0x42);
    CHECK(read6502(0x0300) == 0x42, "test_plain_ram_read_write_roundtrip: $0300");

    /* $2000-$3FFF Hi-Res buffer is plain RAM from apple2_mem.c's
     * perspective -- no special casing, just backing bytes Bunnie's
     * video_apple2.c reads through this same read6502(). */
    write6502(0x2000, 0x99);
    CHECK(read6502(0x2000) == 0x99, "test_plain_ram_read_write_roundtrip: $2000 (Hi-Res buffer)");
}

static void test_rom_region_is_write_protected(void) {
    apple2_mem_reset();

    uint8_t before = read6502(0xF000);
    write6502(0xF000, 0xAB);
    uint8_t after = read6502(0xF000);

    CHECK(after == before, "test_rom_region_is_write_protected: $F000 write ignored");
}

static void test_unknown_soft_switch_reads_zero_and_ignores_writes(void) {
    apple2_mem_reset();

    CHECK(read6502(0xC050) == 0x00,
          "test_unknown_soft_switch_reads_zero_and_ignores_writes: unmapped $C0xx reads 0");

    write6502(0xC050, 0xFF); /* must not crash or corrupt anything */
    CHECK(read6502(0xC050) == 0x00,
          "test_unknown_soft_switch_reads_zero_and_ignores_writes: still 0 after write");
}

static void test_c030_triggers_audio_toggle_without_blocking(void) {
    apple2_mem_reset();
    bunnie_audio_state_t *audio = apple2_mem_get_audio_state();

    CHECK(audio->toggle_pending == 0,
          "test_c030_triggers_audio_toggle_without_blocking: starts with no pending toggle");

    /* Real Apple II hardware toggles on ANY access -- read or write. */
    (void)read6502(0xC030);

    CHECK(audio->toggle_pending != 0,
          "test_c030_triggers_audio_toggle_without_blocking: read6502($C030) sets pending flag");
    CHECK(audio->pwm_pin_state == 0,
          "test_c030_triggers_audio_toggle_without_blocking: trap itself never flips the pin");
}

static void test_disk_trap_select_and_stream_sector_via_c0ec(void) {
    apple2_mem_reset();
    apple2_mem_set_disk_image(g_mock_disk_image);

    write6502(0xC0E0, 17); /* track 17 -- DOS 3.3 VTOC, matches earlier disk_trap tests */
    write6502(0xC0E1, 0);  /* sector 0 -> selects and resets the byte cursor */

    uint32_t base_offset;
    if (dos33_sector_offset(17, 0, &base_offset) != 0) {
        fprintf(stderr, "FAIL: dos33_sector_offset(17,0) unexpectedly rejected\n");
        failures++;
        return;
    }

    int ok = 1;
    for (int i = 0; i < 256; i++) {
        uint8_t got = read6502(0xC0EC);
        uint8_t expected = g_mock_disk_image[base_offset + i];
        if (got != expected) {
            fprintf(stderr,
                    "FAIL: test_disk_trap_select_and_stream_sector_via_c0ec: byte %d = 0x%02X, expected 0x%02X\n",
                    i, got, expected);
            ok = 0;
            failures++;
            break;
        }
    }
    if (ok) {
        printf("PASS: test_disk_trap_select_and_stream_sector_via_c0ec (256/256 bytes streamed correctly)\n");
    }
}

static void test_disk_trap_cursor_wraps_after_256_bytes(void) {
    apple2_mem_reset();
    apple2_mem_set_disk_image(g_mock_disk_image);

    write6502(0xC0E0, 0);
    write6502(0xC0E1, 0);

    for (int i = 0; i < 256; i++) {
        (void)read6502(0xC0EC);
    }
    /* 257th read must wrap back to byte offset 0 of the same sector, not
     * walk off into the next sector or garbage. */
    uint32_t base_offset;
    dos33_sector_offset(0, 0, &base_offset);
    uint8_t wrapped = read6502(0xC0EC);
    CHECK(wrapped == g_mock_disk_image[base_offset],
          "test_disk_trap_cursor_wraps_after_256_bytes");
}

static void test_apple2_mem_reset_clears_mid_stream_disk_cursor(void) {
    /* Same drift-risk class as the paddle-state and button/annunciator
     * reset gaps caught earlier this session: apple2_mem_reset() zeros
     * g_disk_stream_cursor/g_pending_track inline, but nothing proved it
     * actually resets a cursor that's mid-stream (non-zero, partway
     * through a 256-byte sector) at the moment of reset -- every
     * existing disk-trap test only exercises reset->select->stream from
     * a clean start, never a reset happening mid-stream. Confirms a
     * post-reset $C0EC read -- WITHOUT re-selecting via $C0E0/$C0E1,
     * which would trivially reset disk_trap's own cursor regardless of
     * apple2_mem_reset()'s own zeroing -- starts back at cursor 0, not
     * wherever the interrupted stream left off. disk_trap's internal
     * selected-sector state is Duke's module and deliberately NOT reset
     * here (real Apple II hardware doesn't re-home the disk head on a
     * soft reset either) -- only apple2_mem.c's own g_disk_stream_cursor
     * field is under test. */
    apple2_mem_reset();
    apple2_mem_set_disk_image(g_mock_disk_image);
    write6502(0xC0E0, 0);
    write6502(0xC0E1, 0);

    /* Stream partway through the sector -- cursor now sits mid-flight,
     * NOT at 0 and NOT wrapped back around yet. */
    for (int i = 0; i < 100; i++) {
        (void)read6502(0xC0EC);
    }

    apple2_mem_reset();

    /* Deliberately no $C0E0/$C0E1 write here -- if apple2_mem_reset()
     * didn't zero g_disk_stream_cursor, this read would return byte 100
     * of the still-selected sector (continuing where the interrupted
     * stream left off) instead of byte 0. */
    uint32_t base_offset;
    dos33_sector_offset(0, 0, &base_offset);
    uint8_t first_byte_after_reset = read6502(0xC0EC);

    CHECK(first_byte_after_reset == g_mock_disk_image[base_offset],
          "test_apple2_mem_reset_clears_mid_stream_disk_cursor");
}

static void test_disk_trap_invalid_track_select_does_not_disturb_prior_selection(void) {
    apple2_mem_reset();
    apple2_mem_set_disk_image(g_mock_disk_image);

    write6502(0xC0E0, 5);
    write6502(0xC0E1, 3);
    uint8_t baseline = read6502(0xC0EC);

    /* re-select the same sector to reset the cursor, then try an invalid one */
    write6502(0xC0E0, 5);
    write6502(0xC0E1, 3);
    write6502(0xC0E0, 35); /* invalid: DOS33_TRACKS == 35 */
    write6502(0xC0E1, 0);

    uint8_t after_invalid = read6502(0xC0EC);
    CHECK(after_invalid == baseline,
          "test_disk_trap_invalid_track_select_does_not_disturb_prior_selection");
}

/*
 * Language Card ($C080-$C08F) bank-switching tests.
 *
 * Standard Apple II LC softswitch decode (real hardware behavior we're
 * modeling): address bits within $C080-$C08F select read/write mode for
 * the $D000-$FFFF region:
 *   bit0 (0x01): 0 = read RAM, 1 = read ROM
 *   bit1 (0x02) combined with bit0: 00/11 write-enable RAM, 01/10 do not
 *     (i.e. bit0 XOR bit1 == 0 means write-enabled: $C080/$C083/$C088/$C08B)
 *   bit3 (0x08): 0 = bank 2, 1 = bank 1 -- selects one of two independent
 *     4KB RAM banks for $D000-$DFFF ONLY; $E000-$FFFF is a single shared
 *     RAM region regardless of bank bit (matches real Apple II LC wiring:
 *     only $D000-$DFFF is bank-switched, $E000-$FFFF is not).
 *   bit2 (0x04): read-only duplicate of bits 0-1's encoding (C084-C087
 *     behave identically to C080-C083 for our purposes; real hardware's
 *     double-read-to-write-enable latch trick is simplified away here --
 *     a single write access with the write-enable bit pattern takes
 *     effect immediately, documented simplification).
 * Any access (read OR write) to $C080-$C08F triggers the switch, per
 * real Apple II softswitch semantics (same "any access" rule as $C030).
 */

static void test_lc_default_state_reads_rom_and_blocks_writes(void) {
    /* Baseline: before touching any LC softswitch, $D000-$FFFF behaves
     * exactly like the existing ROM write-protection (no regression). */
    apple2_mem_reset();

    uint8_t before = read6502(0xE000);
    write6502(0xE000, 0xAB);
    uint8_t after = read6502(0xE000);

    CHECK(after == before,
          "test_lc_default_state_reads_rom_and_blocks_writes");
}

static void test_lc_c08b_enables_ram_read_and_write_at_d000(void) {
    /* Canonical Apple II Language Card softswitch table (bank 2 shown;
     * add $08 to each address for bank 1, which mirrors the same
     * read/write semantics for a second independent $D000-$DFFF bank):
     *   $C080/$C084 = read RAM,  write protect
     *   $C081/$C085 = read ROM,  write enable
     *   $C082/$C086 = read ROM,  write protect
     *   $C083/$C087 = read RAM,  write enable
     * $C08B = $C083 + $08 = bank 1, read RAM, write enable. */
    apple2_mem_reset();

    (void)read6502(0xC08B); /* bank 1, read RAM, write enable */
    write6502(0xD000, 0x77);
    uint8_t got = read6502(0xD000);

    CHECK(got == 0x77,
          "test_lc_c08b_enables_ram_read_and_write_at_d000");
}

static void test_lc_c082_restores_rom_read_and_write_protection(void) {
    apple2_mem_reset();

    (void)read6502(0xC08B); /* enable RAM read+write, bank 1 */
    write6502(0xD000, 0x77); /* write into LC RAM */

    (void)read6502(0xC082); /* bank 2, read ROM, write protect */
    uint8_t rom_value_before = read6502(0xD000); /* reads ROM, not the 0x77 we wrote to RAM */
    write6502(0xD000, 0x99); /* should be ignored -- ROM write-protected */
    uint8_t after_blocked_write = read6502(0xD000);

    CHECK(rom_value_before != 0x77 && after_blocked_write == rom_value_before,
          "test_lc_c082_restores_rom_read_and_write_protection");
}

static void test_lc_bank1_and_bank2_are_independent_at_d000(void) {
    /* $D000-$DFFF is bank-switched (bit3 selects bank1 vs bank2); a write
     * to bank 1's RAM must NOT be visible when bank 2 is selected, and
     * vice versa -- they are two separate 4KB RAM regions. */
    apple2_mem_reset();

    (void)read6502(0xC08B); /* bank 1 (bit3=1 via $C08B=$0B), RAM read+write */
    write6502(0xD050, 0xAA);

    (void)read6502(0xC083); /* bank 2 (bit3=0 via $C083), RAM read+write */
    write6502(0xD050, 0x55);
    uint8_t bank2_value = read6502(0xD050);

    (void)read6502(0xC08B); /* back to bank 1 */
    uint8_t bank1_value = read6502(0xD050);

    CHECK(bank2_value == 0x55 && bank1_value == 0xAA,
          "test_lc_bank1_and_bank2_are_independent_at_d000");
}

static void test_lc_e000_ffff_is_shared_across_banks(void) {
    /* $E000-$FFFF is NOT bank-switched -- both bank1 and bank2 selections
     * see the same underlying RAM there (only $D000-$DFFF is banked on
     * real Apple II hardware). */
    apple2_mem_reset();

    (void)read6502(0xC08B); /* bank 1, RAM read+write */
    write6502(0xE500, 0x42);

    (void)read6502(0xC083); /* bank 2, RAM read+write */
    uint8_t seen_from_bank2 = read6502(0xE500);

    CHECK(seen_from_bank2 == 0x42,
          "test_lc_e000_ffff_is_shared_across_banks");
}

/*
 * Display-mode softswitch tests ($C050-$C057). Real Apple II semantics:
 * any access (read OR write) to the address triggers the mode change --
 * same "any access" rule as $C030/$C08x already implemented above. Each
 * pair is a simple on/off switch, no latching/toggling logic needed.
 */

static void test_display_mode_defaults_to_text_page1_lores(void) {
    apple2_mem_reset();

    CHECK(apple2_mem_is_text_mode() == 1,
          "test_display_mode_defaults_to_text_page1_lores: TEXT");
    CHECK(apple2_mem_is_mixed_mode() == 0,
          "test_display_mode_defaults_to_text_page1_lores: not MIXED");
    CHECK(apple2_mem_is_page2_selected() == 0,
          "test_display_mode_defaults_to_text_page1_lores: PAGE1");
    CHECK(apple2_mem_is_hires_mode() == 0,
          "test_display_mode_defaults_to_text_page1_lores: LORES");
}

static void test_c050_selects_graphics_mode(void) {
    apple2_mem_reset();
    (void)read6502(0xC050);
    CHECK(apple2_mem_is_text_mode() == 0,
          "test_c050_selects_graphics_mode");
}

static void test_c051_selects_text_mode(void) {
    apple2_mem_reset();
    write6502(0xC050, 0); /* GRAPHICS first */
    write6502(0xC051, 0); /* back to TEXT -- write access must also trigger */
    CHECK(apple2_mem_is_text_mode() == 1,
          "test_c051_selects_text_mode");
}

static void test_c052_selects_full_screen_c053_selects_mixed(void) {
    apple2_mem_reset();
    (void)read6502(0xC053);
    CHECK(apple2_mem_is_mixed_mode() == 1,
          "test_c052_selects_full_screen_c053_selects_mixed: C053 sets MIXED");
    (void)read6502(0xC052);
    CHECK(apple2_mem_is_mixed_mode() == 0,
          "test_c052_selects_full_screen_c053_selects_mixed: C052 clears MIXED");
}

static void test_c054_selects_page1_c055_selects_page2(void) {
    apple2_mem_reset();
    (void)read6502(0xC055);
    CHECK(apple2_mem_is_page2_selected() == 1,
          "test_c054_selects_page1_c055_selects_page2: C055 sets PAGE2");
    (void)read6502(0xC054);
    CHECK(apple2_mem_is_page2_selected() == 0,
          "test_c054_selects_page1_c055_selects_page2: C054 restores PAGE1");
}

static void test_c056_selects_lores_c057_selects_hires(void) {
    apple2_mem_reset();
    (void)read6502(0xC057);
    CHECK(apple2_mem_is_hires_mode() == 1,
          "test_c056_selects_lores_c057_selects_hires: C057 sets HIRES");
    (void)read6502(0xC056);
    CHECK(apple2_mem_is_hires_mode() == 0,
          "test_c056_selects_lores_c057_selects_hires: C056 restores LORES");
}

static void test_display_mode_switches_do_not_disturb_each_other(void) {
    /* TEXT/GRAPHICS, MIXED/FULL, PAGE1/PAGE2, and HIRES/LORES are four
     * INDEPENDENT switches -- flipping one must not touch the others. */
    apple2_mem_reset();

    (void)read6502(0xC050); /* GRAPHICS */
    (void)read6502(0xC053); /* MIXED */
    (void)read6502(0xC055); /* PAGE2 */
    (void)read6502(0xC057); /* HIRES */

    CHECK(apple2_mem_is_text_mode() == 0 && apple2_mem_is_mixed_mode() == 1 &&
          apple2_mem_is_page2_selected() == 1 && apple2_mem_is_hires_mode() == 1,
          "test_display_mode_switches_do_not_disturb_each_other");
}

/*
 * Keyboard input latch ($C000) + strobe clear ($C010) tests. Real Apple
 * II semantics: $C000 read returns the last key's ASCII value with the
 * high bit (0x80) set as the "strobe" flag while a key is pending;
 * ANY access to $C010 (read or write) clears the strobe bit (but the
 * ASCII value itself is left in place until the next keypress, matching
 * real hardware -- only the strobe bit is cleared, not the data).
 */

static void test_keyboard_c000_reads_ascii_with_strobe_bit_set(void) {
    apple2_mem_reset();
    apple2_mem_inject_key('A'); /* 0x41 */

    uint8_t got = read6502(0xC000);
    CHECK(got == (0x41 | 0x80),
          "test_keyboard_c000_reads_ascii_with_strobe_bit_set");
}

static void test_keyboard_c010_clears_strobe_bit(void) {
    apple2_mem_reset();
    apple2_mem_inject_key('B'); /* 0x42 */

    (void)read6502(0xC010); /* any access clears strobe */
    uint8_t after_clear = read6502(0xC000);

    CHECK(after_clear == 0x42, /* strobe (0x80) gone, ASCII value retained */
          "test_keyboard_c010_clears_strobe_bit");
}

static void test_keyboard_c010_write_also_clears_strobe(void) {
    apple2_mem_reset();
    apple2_mem_inject_key('C'); /* 0x43 */

    write6502(0xC010, 0xFF); /* write access must also clear, not just read */
    uint8_t after_clear = read6502(0xC000);

    CHECK(after_clear == 0x43,
          "test_keyboard_c010_write_also_clears_strobe");
}

static void test_keyboard_no_key_pressed_reads_zero(void) {
    apple2_mem_reset();
    CHECK(read6502(0xC000) == 0x00,
          "test_keyboard_no_key_pressed_reads_zero");
}

static void test_keyboard_new_key_resets_strobe_even_before_c010(void) {
    /* A second injected key raises the strobe again with the NEW ascii
     * value, even if the previous key's strobe was never cleared. */
    apple2_mem_reset();
    apple2_mem_inject_key('X');
    apple2_mem_inject_key('Y'); /* overwrite before $C010 was ever touched */

    uint8_t got = read6502(0xC000);
    CHECK(got == ('Y' | 0x80),
          "test_keyboard_new_key_resets_strobe_even_before_c010");
}

/*
 * Pushbutton/paddle input tests ($C061-$C063). Real Apple II semantics:
 * bit 7 (0x80) of the read reflects the button's current held/released
 * state; bits 0-6 are unspecified (we return 0 there, matching the
 * "unimplemented bits read as 0" convention used elsewhere in this file).
 * PB0=$C061, PB1=$C062, PB2=$C063.
 */

static void test_buttons_default_to_released(void) {
    apple2_mem_reset();

    CHECK(read6502(0xC061) == 0x00, "test_buttons_default_to_released: PB0");
    CHECK(read6502(0xC062) == 0x00, "test_buttons_default_to_released: PB1");
    CHECK(read6502(0xC063) == 0x00, "test_buttons_default_to_released: PB2");
}

static void test_pb0_reflects_button0_state(void) {
    apple2_mem_reset();

    apple2_mem_set_button_state(0, 1); /* press button 0 */
    CHECK(read6502(0xC061) == 0x80,
          "test_pb0_reflects_button0_state: pressed reads 0x80");

    apple2_mem_set_button_state(0, 0); /* release */
    CHECK(read6502(0xC061) == 0x00,
          "test_pb0_reflects_button0_state: released reads 0x00");
}

static void test_pb1_reflects_button1_state(void) {
    apple2_mem_reset();

    apple2_mem_set_button_state(1, 1);
    CHECK(read6502(0xC062) == 0x80,
          "test_pb1_reflects_button1_state");
}

static void test_pb2_reflects_button2_state(void) {
    apple2_mem_reset();

    apple2_mem_set_button_state(2, 1);
    CHECK(read6502(0xC063) == 0x80,
          "test_pb2_reflects_button2_state");
}

static void test_buttons_are_independent(void) {
    /* Pressing one button must not affect the others' reported state. */
    apple2_mem_reset();

    apple2_mem_set_button_state(1, 1); /* only button 1 pressed */

    CHECK(read6502(0xC061) == 0x00 && read6502(0xC062) == 0x80 &&
          read6502(0xC063) == 0x00,
          "test_buttons_are_independent");
}

static void test_button_reads_do_not_have_side_effects(void) {
    /* Unlike the other soft-switches in this file, reading a pushbutton
     * address must be a pure query -- it must not toggle/clear/latch
     * anything (repeated reads return the same value, and reading one
     * button must not disturb another). */
    apple2_mem_reset();
    apple2_mem_set_button_state(0, 1);

    uint8_t first = read6502(0xC061);
    uint8_t second = read6502(0xC061);

    CHECK(first == 0x80 && second == 0x80,
          "test_button_reads_do_not_have_side_effects");
}

/*
 * Annunciator (AN0-AN3, $C058-$C05F) tests -- same independent on/off
 * switch pattern as the display-mode softswitches, any access triggers.
 */

static void test_annunciators_default_to_off(void) {
    apple2_mem_reset();

    CHECK(apple2_mem_get_annunciator_state(0) == 0, "test_annunciators_default_to_off: AN0");
    CHECK(apple2_mem_get_annunciator_state(1) == 0, "test_annunciators_default_to_off: AN1");
    CHECK(apple2_mem_get_annunciator_state(2) == 0, "test_annunciators_default_to_off: AN2");
    CHECK(apple2_mem_get_annunciator_state(3) == 0, "test_annunciators_default_to_off: AN3");
}

static void test_an0_c058_off_c059_on(void) {
    apple2_mem_reset();
    (void)read6502(0xC059);
    CHECK(apple2_mem_get_annunciator_state(0) == 1,
          "test_an0_c058_off_c059_on: C059 turns AN0 on");
    write6502(0xC058, 0); /* write access must also trigger */
    CHECK(apple2_mem_get_annunciator_state(0) == 0,
          "test_an0_c058_off_c059_on: C058 turns AN0 off");
}

static void test_an1_c05a_off_c05b_on(void) {
    apple2_mem_reset();
    (void)read6502(0xC05B);
    CHECK(apple2_mem_get_annunciator_state(1) == 1,
          "test_an1_c05a_off_c05b_on");
}

static void test_an2_c05c_off_c05d_on(void) {
    apple2_mem_reset();
    (void)read6502(0xC05D);
    CHECK(apple2_mem_get_annunciator_state(2) == 1,
          "test_an2_c05c_off_c05d_on");
}

static void test_an3_c05e_off_c05f_on(void) {
    apple2_mem_reset();
    (void)read6502(0xC05F);
    CHECK(apple2_mem_get_annunciator_state(3) == 1,
          "test_an3_c05e_off_c05f_on");
}

static void test_annunciators_are_independent(void) {
    apple2_mem_reset();
    (void)read6502(0xC05B); /* only AN1 on */

    CHECK(apple2_mem_get_annunciator_state(0) == 0 &&
          apple2_mem_get_annunciator_state(1) == 1 &&
          apple2_mem_get_annunciator_state(2) == 0 &&
          apple2_mem_get_annunciator_state(3) == 0,
          "test_annunciators_are_independent");
}

/*
 * Paddle analog timer tests ($C064/$C065 PADDLE0/PADDLE1, $C070 PDRIVE).
 * Per apple2_mem.h: $C070 arms an RC countdown; apple2_mem_set_paddle_value()
 * sets how many PADDLEn reads it takes to expire -- 0 = discharges
 * immediately (bit clear on the very first read after arming), higher
 * values take longer, modeling a paddle turned further.
 */

static void test_paddle_with_zero_value_discharges_immediately(void) {
    apple2_mem_reset();
    apple2_mem_set_paddle_value(0, 0);

    (void)read6502(0xC070); /* arm the countdown */
    uint8_t got = read6502(0xC064);

    CHECK((got & 0x80) == 0,
          "test_paddle_with_zero_value_discharges_immediately");
}

static void test_paddle_with_nonzero_value_stays_armed_then_expires(void) {
    apple2_mem_reset();
    apple2_mem_set_paddle_value(0, 2); /* takes 2 reads to expire */

    (void)read6502(0xC070); /* arm */
    uint8_t first_read = read6502(0xC064);
    uint8_t second_read = read6502(0xC064);
    uint8_t third_read = read6502(0xC064);

    CHECK((first_read & 0x80) != 0,
          "test_paddle_with_nonzero_value_stays_armed_then_expires: still armed after 1st read");
    CHECK((second_read & 0x80) != 0,
          "test_paddle_with_nonzero_value_stays_armed_then_expires: still armed after 2nd read");
    CHECK((third_read & 0x80) == 0,
          "test_paddle_with_nonzero_value_stays_armed_then_expires: expired by 3rd read");
}

static void test_paddle0_and_paddle1_are_independent(void) {
    apple2_mem_reset();
    apple2_mem_set_paddle_value(0, 0);   /* PADDLE0 discharges immediately */
    apple2_mem_set_paddle_value(1, 5);   /* PADDLE1 takes a while */

    (void)read6502(0xC070); /* arms BOTH paddles -- one PDRIVE trigger */
    uint8_t paddle0 = read6502(0xC064);
    uint8_t paddle1 = read6502(0xC065);

    CHECK((paddle0 & 0x80) == 0 && (paddle1 & 0x80) != 0,
          "test_paddle0_and_paddle1_are_independent");
}

static void test_paddle_not_armed_reads_zero(void) {
    /* Before any $C070 access, PADDLEn must not report the strobe bit
     * set -- there's no countdown running yet. */
    apple2_mem_reset();
    apple2_mem_set_paddle_value(0, 10);

    uint8_t got = read6502(0xC064);
    CHECK((got & 0x80) == 0,
          "test_paddle_not_armed_reads_zero");
}

int main(void) {
    fill_mock_disk_image();

    test_plain_ram_read_write_roundtrip();
    test_rom_region_is_write_protected();
    test_unknown_soft_switch_reads_zero_and_ignores_writes();
    test_c030_triggers_audio_toggle_without_blocking();
    test_disk_trap_select_and_stream_sector_via_c0ec();
    test_disk_trap_cursor_wraps_after_256_bytes();
    test_apple2_mem_reset_clears_mid_stream_disk_cursor();
    test_disk_trap_invalid_track_select_does_not_disturb_prior_selection();
    test_lc_default_state_reads_rom_and_blocks_writes();
    test_lc_c08b_enables_ram_read_and_write_at_d000();
    test_lc_c082_restores_rom_read_and_write_protection();
    test_lc_bank1_and_bank2_are_independent_at_d000();
    test_lc_e000_ffff_is_shared_across_banks();
    test_display_mode_defaults_to_text_page1_lores();
    test_c050_selects_graphics_mode();
    test_c051_selects_text_mode();
    test_c052_selects_full_screen_c053_selects_mixed();
    test_c054_selects_page1_c055_selects_page2();
    test_c056_selects_lores_c057_selects_hires();
    test_display_mode_switches_do_not_disturb_each_other();
    test_keyboard_c000_reads_ascii_with_strobe_bit_set();
    test_keyboard_c010_clears_strobe_bit();
    test_keyboard_c010_write_also_clears_strobe();
    test_keyboard_no_key_pressed_reads_zero();
    test_keyboard_new_key_resets_strobe_even_before_c010();
    test_buttons_default_to_released();
    test_pb0_reflects_button0_state();
    test_pb1_reflects_button1_state();
    test_pb2_reflects_button2_state();
    test_buttons_are_independent();
    test_button_reads_do_not_have_side_effects();
    test_annunciators_default_to_off();
    test_an0_c058_off_c059_on();
    test_an1_c05a_off_c05b_on();
    test_an2_c05c_off_c05d_on();
    test_an3_c05e_off_c05f_on();
    test_annunciators_are_independent();
    test_paddle_with_zero_value_discharges_immediately();
    test_paddle_with_nonzero_value_stays_armed_then_expires();
    test_paddle0_and_paddle1_are_independent();
    test_paddle_not_armed_reads_zero();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
