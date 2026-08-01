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

int main(void) {
    fill_mock_disk_image();

    test_plain_ram_read_write_roundtrip();
    test_rom_region_is_write_protected();
    test_unknown_soft_switch_reads_zero_and_ignores_writes();
    test_c030_triggers_audio_toggle_without_blocking();
    test_disk_trap_select_and_stream_sector_via_c0ec();
    test_disk_trap_cursor_wraps_after_256_bytes();
    test_disk_trap_invalid_track_select_does_not_disturb_prior_selection();
    test_lc_default_state_reads_rom_and_blocks_writes();
    test_lc_c08b_enables_ram_read_and_write_at_d000();
    test_lc_c082_restores_rom_read_and_write_protection();
    test_lc_bank1_and_bank2_are_independent_at_d000();
    test_lc_e000_ffff_is_shared_across_banks();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
