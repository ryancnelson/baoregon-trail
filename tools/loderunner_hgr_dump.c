/*
 * tools/loderunner_hgr_dump.c -- dumps Lode Runner's real HGR page 1
 * ($2000-$3FFF) content as a decoded PNG-ready RGB buffer, confirming
 * whether it shows real title-screen graphics (not just "non-zero").
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/disk2_controller.h"
#include "../src/apple2_autostart_rom.h"

static void init_system_rom(void) {
    apple2_mem_load_system_rom(g_apple2_autostart_rom);
}

static int load_nib_tracks(const char *dir, disk2_nibble_track_t tracks[DISK2_MAX_TRACKS]) {
    for (int t = 0; t < DISK2_MAX_TRACKS; t++) {
        char path[512];
        snprintf(path, sizeof(path), "%s/track%02d.nib", dir, t);
        FILE *f = fopen(path, "rb");
        if (!f) { fprintf(stderr, "error: cannot open %s\n", path); return 1; }
        size_t got = fread(tracks[t].data, 1, DISK2_MAX_TRACK_BYTES, f);
        fclose(f);
        tracks[t].length = (int)got;
        if (got == 0) { fprintf(stderr, "error: %s is empty\n", path); return 1; }
    }
    return 0;
}

/* Real Apple II HGR row-interleave table (192 rows, same non-linear
 * layout as text/lores but for the larger 8192-byte HGR page). */
static uint16_t hgr_row_offset(int row) {
    int block = row / 64;      /* 0, 1, or 2 */
    int in_block = row % 64;
    int group = in_block / 8;  /* 0-7 */
    int line = in_block % 8;   /* 0-7 */
    return (uint16_t)(block * 0x28 + group * 0x80 + line * 0x400);
}

int main(int argc, char **argv) {
    const char *nib_dir = (argc > 1) ? argv[1] : "/tmp/loderunner_4am_nib";
    uint64_t cycles = (argc > 2) ? strtoull(argv[2], NULL, 10) : 1000000000ull;
    const char *out_path = (argc > 3) ? argv[3] : "/tmp/lr_hgr_raw.bin";

    static disk2_nibble_track_t tracks[DISK2_MAX_TRACKS];
    if (load_nib_tracks(nib_dir, tracks) != 0) return 1;

    apple2_mem_reset();
    reset6502();
    init_system_rom();
    apple2_mem_set_disk_controller_mode(APPLE2_MEM_DISK_CONTROLLER_DISK2);
    disk2_controller_t *ctl = apple2_mem_get_disk2_controller();
    disk2_controller_load_nibble_disk(ctl, 0, tracks, 0);

    pc = 0xC600; x = 0x60; a = 0x60;

    uint64_t executed = 0;
    while (executed < cycles) {
        exec6502(1000);
        executed += 1000;
    }

    fprintf(stderr, "Ran %llu cycles, dumping HGR page 1 as 280x192 8-bit-per-pixel "
                    "(0 or 255, MSB-per-byte ignored, 7 pixels/byte) grayscale raw.\n",
            (unsigned long long)executed);

    FILE *out = fopen(out_path, "wb");
    if (!out) { fprintf(stderr, "cannot open %s for write\n", out_path); return 1; }

    uint8_t row_pixels[280];
    for (int row = 0; row < 192; row++) {
        uint16_t base = (uint16_t)(0x2000 + hgr_row_offset(row));
        int col = 0;
        for (int byte_i = 0; byte_i < 40 && col < 280; byte_i++) {
            uint8_t b = read6502((uint16_t)(base + byte_i));
            for (int bit = 0; bit < 7 && col < 280; bit++) {
                row_pixels[col++] = (b & (1 << bit)) ? 255 : 0;
            }
        }
        fwrite(row_pixels, 1, 280, out);
    }
    fclose(out);
    fprintf(stderr, "Wrote %s (280x192, 8bpp grayscale raw, no header)\n", out_path);
    return 0;
}
