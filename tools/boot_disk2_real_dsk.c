/*
 * tools/boot_disk2_real_dsk.c -- boots a REAL, unmodified DOS 3.3 disk
 * image (Apple_DOS_3.3_Master.dsk or Zork_I.dsk) through
 * baoregon-trail's disk2_controller.c real Disk II emulation
 * (NEXT_STEPS.md Step 7), entering directly at the boot PROM's own
 * entry point ($C600) rather than through a full Apple IIe system ROM
 * reset vector -- avoids the still-unsolved system-ROM-mapping mystery
 * documented in tools/fixtures/mame-captures/README.md (Woz's domain,
 * out of scope here) and directly exercises disk2_controller.c's own
 * boot ROM + softswitch + nibble-read pipeline in isolation.
 *
 * Usage: boot_disk2_real_dsk <nib_track_dir> [cycles] [expect_string]
 *   nib_track_dir is a directory of track00.nib .. track34.nib files,
 *   produced by: python3 tools/dsk_to_nib.py <dsk_path> -o <nib_track_dir>
 *
 * Success criterion (matching tools/fixtures/mame-captures/README.md's
 * own real-MAME-verified reference): after a successful DOS 3.3 boot,
 * text screen memory ($0400-$07FF) contains the literal string
 * "DOS VERSION 3.3" (using the Apple II's own high-bit-set ASCII
 * encoding for displayed characters).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/disk2_controller.h"

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

/* Screen text memory is stored with the Apple II's own high-bit-set
 * "normal" ASCII encoding (0xC0-0xDF for uppercase letters, etc.) --
 * mask off the high bit before printable comparison. */
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
    uint32_t cycles = (argc > 2) ? (uint32_t)atol(argv[2]) : 2000000;
    const char *expect = (argc > 3) ? argv[3] : "DOS VERSION 3.3";

    static disk2_nibble_track_t tracks[DISK2_MAX_TRACKS];
    if (load_nib_tracks(nib_dir, tracks) != 0) {
        return 1;
    }

    apple2_mem_reset();
    reset6502();
    apple2_mem_set_disk_controller_mode(APPLE2_MEM_DISK_CONTROLLER_DISK2);

    disk2_controller_t *ctl = apple2_mem_get_disk2_controller();
    disk2_controller_load_nibble_disk(ctl, 0, tracks, 0);

    /* Enter directly at the boot PROM's own entry point ($C600, slot 6)
     * -- this is exactly where real Apple II system ROM autostart code
     * JSRs on cold boot for the lowest-numbered bootable slot. Skips
     * needing a full system ROM image (unresolved mapping mystery, see
     * file header) since we're testing disk2_controller.c's own domain
     * in isolation. */
    pc = 0xC600;

    fprintf(stderr, "Starting execution at $C600 (Disk II boot PROM entry point), %u cycles budget...\n", cycles);
    uint32_t total_executed = 0;
    uint32_t chunk = 10000;
    uint16_t last_pc = pc;
    int stuck_count = 0;
    while (total_executed < cycles) {
        uint16_t pc_before = pc;
        exec6502(chunk);
        total_executed += chunk;
        if (pc == pc_before) {
            stuck_count++;
        } else {
            stuck_count = 0;
        }
        last_pc = pc;
        if (stuck_count > 20) {
            fprintf(stderr, "PC stuck at $%04X after %u cycles -- stopping early\n", last_pc, total_executed);
            break;
        }
    }

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
