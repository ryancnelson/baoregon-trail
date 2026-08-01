/*
 * dump_framebuffer.c -- initializes the emulator (via emulator_loop.c),
 * runs it forward some number of frames, then dumps
 * baoregon_emulator_get_framebuffer() to a raw RGB565 file for
 * tools/fb_terminal_viewer.c to render. Closes the loop on
 * NEXT_STEPS.md Step 4's "verify rendering of Apple II screen buffers"
 * checklist item end-to-end: run the real emulator -> dump its
 * framebuffer -> view it in a terminal.
 *
 * Usage: dump_framebuffer <output.raw> [num_frames]
 *   num_frames defaults to 1. Splash-menu frames are LORES by default
 *   (real Apple II post-reset state); to see something more interesting,
 *   inject a disk image and press SELECT first via a custom harness, or
 *   just use this to sanity-check the splash screen's Lo-Res rendering
 *   pipeline is wired up correctly end-to-end.
 */
#include <stdio.h>
#include <stdlib.h>

#include "../src/emulator_loop.h"
#include "../src/bio_display.h"

int main(int argc, char **argv) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: %s <output.raw> [num_frames]\n", argv[0]);
        return 1;
    }

    int num_frames = 1;
    if (argc == 3) {
        num_frames = atoi(argv[2]);
        if (num_frames < 1) {
            fprintf(stderr, "error: num_frames must be >= 1\n");
            return 1;
        }
    }

    baoregon_emulator_init();
    for (int i = 0; i < num_frames; i++) {
        baoregon_emulator_run_frame();
    }

    const uint16_t *fb = baoregon_emulator_get_framebuffer();

    FILE *out = fopen(argv[1], "wb");
    if (!out) {
        perror("fopen");
        return 1;
    }

    size_t want = BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT;
    size_t written = fwrite(fb, sizeof(uint16_t), want, out);
    fclose(out);

    if (written != want) {
        fprintf(stderr, "error: wrote %zu of %zu uint16_t values\n", written, want);
        return 1;
    }

    fprintf(stderr, "Wrote %dx%d RGB565 framebuffer (%zu bytes) to %s after %d frame(s).\n",
            BIO_DISPLAY_WIDTH, BIO_DISPLAY_HEIGHT, want * sizeof(uint16_t), argv[1], num_frames);
    return 0;
}
