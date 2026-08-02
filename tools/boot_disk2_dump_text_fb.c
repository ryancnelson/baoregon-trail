/*
 * tools/boot_disk2_dump_text_fb.c -- same boot as
 * boot_disk2_and_render_text.c, but dumps the raw RGB565 framebuffer to
 * a file for programmatic verification instead of ANSI terminal print.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/disk2_controller.h"
#include "../src/text_apple2.h"
#include "../src/bio_display.h"

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

#define SYSTEM_ROM_SIZE 16384
static uint8_t g_stub_rom[SYSTEM_ROM_SIZE];

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <nib_track_dir> <out.raw> [cycles]\n", argv[0]);
        return 1;
    }
    const char *nib_dir = argv[1];
    const char *out_path = argv[2];
    uint32_t cycles = (argc > 3) ? (uint32_t)atol(argv[3]) : 5000000;

    static disk2_nibble_track_t tracks[DISK2_MAX_TRACKS];
    if (load_nib_tracks(nib_dir, tracks) != 0) return 1;

    memset(g_stub_rom, 0x60, sizeof(g_stub_rom));
    g_stub_rom[0x3CA8] = 0xA9;
    g_stub_rom[0x3CA9] = 0x00;
    g_stub_rom[0x3CAA] = 0x60;

    apple2_mem_reset();
    reset6502();
    apple2_mem_load_system_rom(g_stub_rom);
    apple2_mem_set_disk_controller_mode(APPLE2_MEM_DISK_CONTROLLER_DISK2);

    disk2_controller_t *ctl = apple2_mem_get_disk2_controller();
    disk2_controller_load_nibble_disk(ctl, 0, tracks, 0);

    pc = 0xC600;
    x = 0x60;
    a = 0x60;

    uint32_t total_executed = 0;
    uint32_t chunk = 10000;
    while (total_executed < cycles) {
        exec6502(chunk);
        total_executed += chunk;
    }
    fprintf(stderr, "Executed %u cycles. Final PC=$%04X\n", total_executed, pc);

    static uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    text_apple2_render_frame(0, read6502, framebuffer);

    FILE *out = fopen(out_path, "wb");
    if (!out) { fprintf(stderr, "error: cannot write %s\n", out_path); return 1; }
    fwrite(framebuffer, sizeof(uint16_t), BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT, out);
    fclose(out);
    printf("Wrote %s\n", out_path);
    return 0;
}
