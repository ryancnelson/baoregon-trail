/*
 * tools/loderunner_altrom_boot_keypress.c -- variant of
 * loderunner_altrom_boot.c that periodically injects a real keypress
 * (matching Duke's keypress hypothesis being tested for Zork) to check
 * whether Lode Runner's boot is similarly blocked waiting on input,
 * and dumps HGR/graphics-mode softswitch state at the end.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/disk2_controller.h"
#include "../build-scratch/alt_rom_asoft_auto.h"

#define SYSTEM_ROM_SIZE 16384
static uint8_t g_system_rom[SYSTEM_ROM_SIZE];

static void init_system_rom(void) {
    for (int i = 0; i < SYSTEM_ROM_SIZE; i++) {
        g_system_rom[i] = g_alt_rom_asoft_auto[i];
    }
}

static int load_nib_tracks(const char *dir, disk2_nibble_track_t tracks[DISK2_MAX_TRACKS]) {
    for (int t = 0; t < DISK2_MAX_TRACKS; t++) {
        char path[512];
        snprintf(path, sizeof(path), "%s/track%02d.nib", dir, t);
        FILE *f = fopen(path, "rb");
        if (!f) {
            fprintf(stderr, "error: cannot open %s\n", path);
            return 1;
        }
        size_t got = fread(tracks[t].data, 1, DISK2_MAX_TRACK_BYTES, f);
        fclose(f);
        tracks[t].length = (int)got;
        if (got == 0) {
            fprintf(stderr, "error: %s is empty\n", path);
            return 1;
        }
    }
    return 0;
}

static void print_screen_text(void) {
    static const uint16_t row_offsets[24] = {
        0x0000, 0x0080, 0x0100, 0x0180, 0x0200, 0x0280, 0x0300, 0x0380,
        0x0028, 0x00A8, 0x0128, 0x01A8, 0x0228, 0x02A8, 0x0328, 0x03A8,
        0x0050, 0x00D0, 0x0150, 0x01D0, 0x0250, 0x02D0, 0x0350, 0x03D0,
    };
    printf("=== Screen memory (Page 1, $0400), high bit masked off ===\n");
    for (int row = 0; row < 24; row++) {
        char line[41];
        for (int col = 0; col < 40; col++) {
            uint8_t b = read6502((uint16_t)(0x0400 + row_offsets[row] + col));
            uint8_t ch = b & 0x7F;
            line[col] = (ch >= 0x20 && ch < 0x7F) ? (char)ch : '.';
        }
        line[40] = '\0';
        printf("%2d: %s\n", row, line);
    }
}

static void dump_mem_range(uint16_t start, uint16_t len) {
    printf("=== Memory dump $%04X-$%04X ===\n", start, (uint16_t)(start + len - 1));
    for (uint16_t i = 0; i < len; i++) {
        if (i % 16 == 0) printf("$%04X: ", (uint16_t)(start + i));
        printf("%02X ", read6502((uint16_t)(start + i)));
        if (i % 16 == 15) printf("\n");
    }
    printf("\n");
}

/* Checks whether HGR (Hi-Res Graphics) page 1 ($2000-$3FFF) has any
 * non-zero content -- if Lode Runner switched to graphics mode via
 * $C050/$C052/$C054/$C057, its actual "screen" would be there, not in
 * the text page this harness's print_screen_text() checks. */
static int hgr_page1_has_content(void) {
    for (uint32_t addr = 0x2000; addr <= 0x3FFF; addr++) {
        if (read6502((uint16_t)addr) != 0) return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    const char *nib_dir = (argc > 1) ? argv[1] : "/tmp/loderunner_4am_nib";
    uint64_t cycles = (argc > 2) ? strtoull(argv[2], NULL, 10) : 300000000ull;
    uint64_t keypress_at = (argc > 3) ? strtoull(argv[3], NULL, 10) : 0ull; /* 0 = no keypress */
    uint8_t keypress_ascii = (argc > 4) ? (uint8_t)strtoul(argv[4], NULL, 0) : 0x0D; /* default RETURN */

    static disk2_nibble_track_t tracks[DISK2_MAX_TRACKS];
    if (load_nib_tracks(nib_dir, tracks) != 0) {
        return 1;
    }

    apple2_mem_reset();
    reset6502();
    init_system_rom();
    apple2_mem_load_system_rom(g_system_rom);

    apple2_mem_set_disk_controller_mode(APPLE2_MEM_DISK_CONTROLLER_DISK2);
    disk2_controller_t *ctl = apple2_mem_get_disk2_controller();
    disk2_controller_load_nibble_disk(ctl, 0, tracks, 0);

    pc = 0xC600;
    x = 0x60;
    a = 0x60;

    fprintf(stderr, "Booting Lode Runner (%s) via apple2-asoft-auto.rom, %llu cycles budget, "
                    "keypress 0x%02X at cycle %llu (0=disabled)...\n",
            nib_dir, (unsigned long long)cycles, keypress_ascii, (unsigned long long)keypress_at);

    uint64_t executed = 0;
    const uint64_t chunk = 1000ull;
    uint16_t last_pc = pc;
    uint64_t stuck_since = 0;
    uint16_t stuck_at_pc = 0;
    int keypress_sent = 0;
    while (executed < cycles) {
        exec6502((uint32_t)chunk);
        executed += chunk;
        if (pc != last_pc) {
            stuck_since = executed;
            stuck_at_pc = pc;
        }
        last_pc = pc;
        if (executed % 50000000ull < chunk) {
            fprintf(stderr, "  ...%llu cycles, PC=$%04X, SP=$%02X, motor=%d\n",
                    (unsigned long long)executed, pc, sp, ctl->motor_on);
        }
        if (!keypress_sent && keypress_at > 0 && executed >= keypress_at) {
            apple2_mem_inject_key(keypress_ascii);
            keypress_sent = 1;
            fprintf(stderr, "  [injected keypress 0x%02X at %llu cycles]\n",
                    keypress_ascii, (unsigned long long)executed);
        }
    }

    fprintf(stderr, "\n=== FINAL STATE after %llu cycles ===\n", (unsigned long long)executed);
    fprintf(stderr, "PC=$%04X A=$%02X X=$%02X Y=$%02X SP=$%02X\n", pc, a, x, y, sp);
    fprintf(stderr, "Last stable PC=$%04X (no change since %llu cycles, i.e. %llu cycles of no PC movement)\n",
            stuck_at_pc, (unsigned long long)stuck_since, (unsigned long long)(executed - stuck_since));
    fprintf(stderr, "disk2_controller: motor_on=%d track=%d head=%d\n",
            ctl->motor_on, ctl->drive[0].track, ctl->drive[0].head);
    fprintf(stderr, "HGR page 1 ($2000-$3FFF) has non-zero content: %s\n",
            hgr_page1_has_content() ? "YES" : "no");

    print_screen_text();
    dump_mem_range(0x6050, 32);
    dump_mem_range(0xFFF0, 16);

    return 0;
}
