/*
 * tools/disk_swap_cycle_budget_test.c -- re-tests Duke's reframing
 * (commit ad5e126, NEXT_STEPS.md "DUKE'S THREE-DISK RE-TEST"): does the
 * DOS-3.3-boots-then-swaps-to-Zork CATALOG scenario actually reach a
 * real DRVERR/error message given a MUCH larger cycle budget (2-20
 * billion cycles instead of the ~200,000,000 used previously), or does
 * it genuinely hang forever regardless of budget?
 *
 * Real project code path, matching NEXT_STEPS.md's own confirmed-working
 * recipe exactly (the "FABLE HANDOFF" / pivot-plan section): boots
 * against the real apple2-asoft-auto.rom (Apple II+ Autostart ROM with
 * genuine Applesoft BASIC, verified authentic via
 * tools/test_alt_rom_scratch.py's chunk-by-chunk SHA1 check against
 * MAME's own apple2p romset -- NOT the project's real
 * src/apple2e_system_rom.h, which is Monitor-ROM-only with no
 * Applesoft) -- this is the ROM/disk combination NEXT_STEPS.md
 * documents as reaching a genuine stable ']' Applesoft prompt after
 * ~200M cycles + one injected RETURN (real DOS 3.3 Master pauses
 * mid-boot at a real "DISK VOLUME 254" RDKEY-wait prompt, $FD1B-$FD1F,
 * not a bug -- needs an explicit keypress to continue).
 *
 *   1. Boot DOS 3.3 Master to a stable ']' prompt (cycles_budget_1 +
 *      one injected RETURN mid-boot for the DISK VOLUME 254 pause).
 *   2. disk2_controller_load_nibble_disk() swaps drive 0's tracks to
 *      Zork I (tools/zork1_4amcrack.dsk, freshly nibblized this run --
 *      Ryan's priority pick) WITHOUT resetting the 6502 or the disk
 *      controller -- exactly what real Apple II users did.
 *   3. apple2_mem_inject_key() types "CATALOG" + RETURN.
 *   4. Run a MUCH larger cycle budget and report the FINAL state: PC,
 *      motor state, drive track, and the real screen-memory text
 *      content -- not a guess, the actual bytes.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/disk2_controller.h"
#include "../build-scratch/alt_rom_asoft_auto.h"
#include "../src/dos33_master_nib_disk_data.h"

#define SYSTEM_ROM_SIZE 16384
static uint8_t g_system_rom[SYSTEM_ROM_SIZE];

static void init_system_rom(void) {
    /* Use the alt ROM AS-IS -- real Apple II+ Autostart ROM with genuine
     * working code at all Monitor entry points and real Applesoft BASIC.
     * No patches needed (unlike src/main_qemu_dos33boot.c's
     * Monitor-only-ROM path), matching tools/dos33boot_altrom_scratch.c
     * exactly. */
    for (int i = 0; i < SYSTEM_ROM_SIZE; i++) {
        g_system_rom[i] = g_alt_rom_asoft_auto[i];
    }
}

static disk2_nibble_track_t g_dos33_tracks[DISK2_MAX_TRACKS];

static void load_dos33_master(void) {
    for (int t = 0; t < G_DOS33_MASTER_TRACKS_NUM_TRACKS; t++) {
        for (int b = 0; b < G_DOS33_MASTER_TRACKS_MAX_TRACK_BYTES; b++) {
            g_dos33_tracks[t].data[b] = g_dos33_master_tracks_track_data[t][b];
        }
        g_dos33_tracks[t].length = g_dos33_master_tracks_track_lengths[t];
    }
}

/* Load Zork's nibble tracks from raw files at runtime (host build only
 * -- avoids needing a second multi-MB generated header just for this
 * one-shot investigation script). Matches tools/dsk_to_nib.py's
 * trackNN.nib naming exactly. */
static int load_zork_tracks_from_dir(const char *dir, disk2_nibble_track_t tracks[DISK2_MAX_TRACKS]) {
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

int main(int argc, char **argv) {
    /* uint64_t, not uint32_t -- an earlier version of this tool used
     * uint32_t here and silently overflowed/truncated a requested
     * 20,000,000,000-cycle run down to ~2.82 billion (20000000000 mod
     * 2^32) without any error, a real bug caught by comparing the
     * logged final cycle count against the requested one. Fixed so
     * the full requested range up to and beyond 20 billion actually
     * runs as specified. */
    uint64_t boot_cycles = (argc > 1) ? strtoull(argv[1], NULL, 10) : 220000000ull;
    uint64_t post_swap_cycles = (argc > 2) ? strtoull(argv[2], NULL, 10) : 2000000000ull;
    const char *zork_nib_dir = (argc > 3) ? argv[3] : "/tmp/zork1_4amcrack_nib";

    fprintf(stderr, "Boot cycles: %llu, post-swap CATALOG cycles: %llu, zork dir: %s\n",
            (unsigned long long)boot_cycles, (unsigned long long)post_swap_cycles, zork_nib_dir);

    static disk2_nibble_track_t zork_tracks[DISK2_MAX_TRACKS];
    if (load_zork_tracks_from_dir(zork_nib_dir, zork_tracks) != 0) {
        return 1;
    }

    apple2_mem_reset();
    reset6502();
    init_system_rom();
    apple2_mem_load_system_rom(g_system_rom);

    apple2_mem_set_disk_controller_mode(APPLE2_MEM_DISK_CONTROLLER_DISK2);
    disk2_controller_t *ctl = apple2_mem_get_disk2_controller();
    load_dos33_master();
    disk2_controller_load_nibble_disk(ctl, 0, g_dos33_tracks, 0);

    pc = 0xC600;
    x = 0x60;
    a = 0x60;

    fprintf(stderr, "Phase 1: booting DOS 3.3 Master with apple2-asoft-auto.rom (%llu cycles, "
                    "with one injected RETURN mid-boot for the real DISK VOLUME 254 RDKEY-wait pause)...\n",
            (unsigned long long)boot_cycles);
    uint64_t executed = 0;
    const uint64_t chunk = 20000ull;
    int mid_boot_return_sent = 0;
    while (executed < boot_cycles) {
        exec6502((uint32_t)chunk);
        executed += chunk;
        /* Real DOS 3.3 Master pauses mid-boot at a genuine "DISK VOLUME
         * 254" RDKEY-wait prompt around the halfway point -- inject one
         * RETURN partway through the boot budget to get past it,
         * matching NEXT_STEPS.md's documented working recipe. */
        if (!mid_boot_return_sent && executed >= boot_cycles / 2) {
            apple2_mem_inject_key(0x8D);
            mid_boot_return_sent = 1;
            fprintf(stderr, "  [injected RETURN at %llu cycles for DISK VOLUME 254 pause]\n",
                    (unsigned long long)executed);
        }
    }
    fprintf(stderr, "Phase 1 done. PC=$%04X\n", pc);
    print_screen_text();

    fprintf(stderr, "\nPhase 2: swapping drive 0 to Zork I (%s), NO reset...\n", zork_nib_dir);
    disk2_controller_load_nibble_disk(ctl, 0, zork_tracks, 0);

    fprintf(stderr, "Phase 3: injecting \"CATALOG\" + RETURN...\n");
    const char *cmd = "CATALOG";
    for (const char *p = cmd; *p; p++) {
        apple2_mem_inject_key((uint8_t)(*p | 0x80));
    }
    apple2_mem_inject_key(0x8D);

    fprintf(stderr, "Phase 4: running %llu cycles (this is the REAL TEST -- Duke's much-larger budget)...\n",
            (unsigned long long)post_swap_cycles);
    executed = 0;
    uint64_t last_pc_report_at = 0;
    while (executed < post_swap_cycles) {
        exec6502((uint32_t)chunk);
        executed += chunk;
        if (executed - last_pc_report_at >= 200000000ull) {
            fprintf(stderr, "  ...%llu cycles executed, PC=$%04X, drive0.track=%d, drive0.head=%d, motor_on=%d\n",
                    (unsigned long long)executed, pc, ctl->drive[0].track, ctl->drive[0].head, ctl->motor_on);
            last_pc_report_at = executed;
        }
    }

    fprintf(stderr, "\n=== FINAL STATE after %llu post-swap cycles ===\n", (unsigned long long)post_swap_cycles);
    fprintf(stderr, "PC=$%04X A=$%02X X=$%02X Y=$%02X SP=$%02X\n", pc, a, x, y, sp);
    fprintf(stderr, "disk2_controller: motor_on=%d q6=%d q7=%d selected_drive=%d latch=$%02X\n",
            ctl->motor_on, ctl->q6, ctl->q7, ctl->selected_drive, ctl->latch);
    fprintf(stderr, "drive[0]: track=%d head=%d has_disk=%d\n",
            ctl->drive[0].track, ctl->drive[0].head, ctl->drive[0].has_disk);
    uint8_t track_shadow = read6502(0x0478);
    fprintf(stderr, "zero-page $0478 (RWTS track-shadow variable): %d ($%02X)\n", track_shadow, track_shadow);

    printf("\n");
    print_screen_text();

    return 0;
}
