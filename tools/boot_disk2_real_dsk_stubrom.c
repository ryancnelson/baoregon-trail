/*
 * tools/boot_disk2_real_dsk_stubrom.c -- variant of boot_disk2_real_dsk.c
 * that loads a MINIMAL synthetic 16KB system ROM stub (all bytes 0x60 =
 * RTS) via apple2_mem_load_system_rom(), rather than entering at $C600
 * directly with no system ROM at all.
 *
 * WHY: real Disk II boot PROM code (341-0027-a.p5) calls JSR $FCA8 (the
 * real Apple II monitor ROM's WAIT subroutine, a busy-wait delay used
 * to let the drive motor spin up before the first read attempt) partway
 * through its own boot sequence. Without ANY system ROM loaded, that
 * address reads as zeroed RAM and the CPU crashes into garbage
 * execution. Since real timing-accurate delay isn't the point of this
 * test (matches this project's existing "documented simplification"
 * precedent for other un-timed softswitches, e.g. Disk II stepper motor
 * timing in disk2_controller.c itself, paddle RC countdown in
 * apple2_mem.c), a MINIMAL stub ROM (every byte = 0x60/RTS) lets any
 * JSR into ROM territory return immediately rather than crash --
 * good enough to prove the disk2_controller.c boot pipeline itself
 * works, without needing a full, real, correctly-mapped Apple IIe
 * system ROM (a separate, as-yet-unsolved problem -- see
 * tools/fixtures/mame-captures/README.md).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/disk2_controller.h"

#define SYSTEM_ROM_SIZE 16384

static uint8_t g_stub_rom[SYSTEM_ROM_SIZE];

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

static void extract_screen_text(char *out, size_t out_len) {
    size_t o = 0;
    for (uint16_t addr = 0x0400; addr <= 0x07FF && o + 1 < out_len; addr++) {
        uint8_t b = (uint8_t)(read6502(addr) & 0x7F);
        if (b >= 0x20 && b < 0x7F) {
            out[o++] = (char)b;
        } else if (b == 0x00 || b == 0x0D) {
            out[o++] = ' ';
        }
    }
    out[o] = '\0';
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <nib_track_dir> [cycles] [expect_string]\n", argv[0]);
        return 1;
    }
    const char *nib_dir = argv[1];
    uint32_t cycles = (argc > 2) ? (uint32_t)atol(argv[2]) : 5000000;
    const char *expect = (argc > 3) ? argv[3] : "DOS VERSION 3.3";

    static disk2_nibble_track_t tracks[DISK2_MAX_TRACKS];
    if (load_nib_tracks(nib_dir, tracks) != 0) {
        return 1;
    }

    /* Minimal stub ROM: every byte is 0x60 (RTS), so any JSR into ROM
     * territory (like the boot PROM's JSR $FCA8) returns immediately
     * instead of crashing into zeroed-RAM garbage. Not a real monitor
     * ROM -- just enough to let boot flow continue. */
    memset(g_stub_rom, 0x60, sizeof(g_stub_rom));

    apple2_mem_reset();
    reset6502();
    apple2_mem_load_system_rom(g_stub_rom);
    apple2_mem_set_disk_controller_mode(APPLE2_MEM_DISK_CONTROLLER_DISK2);

    disk2_controller_t *ctl = apple2_mem_get_disk2_controller();
    disk2_controller_load_nibble_disk(ctl, 0, tracks, 0);

    pc = 0xC600;
    /* Real Apple II calling convention (corrected after inspecting the
     * actual stuck loop at $C65E: `LDA $C08C,X` -- this needs X = 0x60
     * so that $C08C+X lands at $C0EC, matching slot 6's real softswitch
     * base $C080 + slot*0x10 = $C080 + 0x60 = $C0E0). Both A and X hold
     * slot*16 on entry, not the plain slot number -- an initial guess
     * of X=6 (plain slot number) left the boot PROM's own $C08C,X-style
     * addressing pointing at the Language Card softswitch range instead
     * of the Disk II range, causing an infinite wait for a sync byte
     * that could never appear. */
    x = 0x60;
    a = 0x60;

    fprintf(stderr, "Starting execution at $C600 (Disk II boot PROM entry point) with stub ROM, %u cycles budget...\n", cycles);
    /* Diagnostic: dump the first 32 raw bytes of track 0's nibble data
     * as the controller sees them, to confirm gap/sync bytes are
     * actually present at head=0 before blaming the CPU loop. */
    fprintf(stderr, "track[0].length=%d, first 16 bytes: ", tracks[0].length);
    for (int i = 0; i < 16 && i < tracks[0].length; i++) {
        fprintf(stderr, "%02X ", tracks[0].data[i]);
    }
    fprintf(stderr, "\n");

    uint32_t total_executed = 0;
    uint32_t chunk = 1;
    uint16_t last_pc = pc;
    int stuck_count = 0;
    uint16_t trace_pcs[20];
    int trace_idx = 0;
    while (total_executed < cycles) {
        uint16_t pc_before = pc;
        exec6502(chunk);
        total_executed += chunk;
        if (pc == pc_before) {
            stuck_count++;
        } else {
            stuck_count = 0;
        }
        if (total_executed > cycles - 200) {
            trace_pcs[trace_idx % 20] = pc;
            trace_idx++;
        }
        last_pc = pc;
        if (stuck_count > 2000) {
            fprintf(stderr, "PC stuck at $%04X after %u cycles -- stopping early\n", last_pc, total_executed);
            break;
        }
    }
    fprintf(stderr, "Last %d PCs before stopping: ", trace_idx < 20 ? trace_idx : 20);
    for (int i = 0; i < (trace_idx < 20 ? trace_idx : 20); i++) {
        fprintf(stderr, "$%04X ", trace_pcs[i]);
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "disk2 controller state: motor_on=%d q6=%d q7=%d selected_drive=%d latch=0x%02X\n",
            ctl->motor_on, ctl->q6, ctl->q7, ctl->selected_drive, ctl->latch);
    fprintf(stderr, "drive[0] state: track=%d head=%d skip=%d has_disk=%d\n",
            ctl->drive[0].track, ctl->drive[0].head, ctl->drive[0].skip, ctl->drive[0].has_disk);

    fprintf(stderr, "Executed %u cycles. Final PC=$%04X\n", total_executed, last_pc);

    char screen_text[2048];
    extract_screen_text(screen_text, sizeof(screen_text));
    fprintf(stderr, "Screen text (raw): %s\n", screen_text);

    if (strstr(screen_text, expect) != NULL) {
        printf("PASS: found expected string \"%s\" in screen memory -- real DOS 3.3 boot confirmed working through disk2_controller.c\n", expect);
        return 0;
    } else {
        printf("FAIL: expected string \"%s\" NOT found in screen memory\n", expect);
        return 1;
    }
}
