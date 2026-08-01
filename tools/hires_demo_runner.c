/*
 * tools/hires_demo_runner.c -- Standalone host runner that boots
 * disks/hires_demo.dsk's bootloader directly (bypassing the boot-splash
 * cartridge-slot menu, which has no game content loaded yet) and renders
 * the resulting Hi-Res framebuffer live in the terminal.
 *
 * Unlike terminal_emulator.c (which goes through the full splash-menu ->
 * game-select flow), this loads the boot sector directly into $0800 and
 * jumps to it -- the same technique tests/test_dos33_composed_boot.c uses,
 * just continuously rendered instead of asserted-once.
 *
 * Demonstrates the Hi-Res COLOR rendering pipeline (video_apple2.c /
 * bio_display.c), which is implemented and tested -- unlike TEXT mode,
 * which currently always renders black (no character-ROM renderer exists
 * yet; see src/bio_display.c's documented gap).
 *
 * Usage: hires_demo_runner [disk_path] [max_frames]
 *   disk_path defaults to disks/hires_demo.dsk
 *   max_frames: 0 (default) = run until 'q'/ESC in interactive mode, or
 *               forever in non-interactive/piped mode (use a positive
 *               number to cap it in scripts/CI).
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <stdint.h>

#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/bio_display.h"

void fb_terminal_viewer_print(const uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]);

static struct termios orig_termios;

static void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf("\x1b[0m\x1b[?25h\n");
}

static void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    printf("\x1b[?25l\x1b[2J\x1b[H");
}

int main(int argc, char **argv) {
    const char *disk_path = (argc > 1) ? argv[1] : "disks/hires_demo.dsk";
    int max_frames = (argc > 2) ? atoi(argv[2]) : 0;

    FILE *f = fopen(disk_path, "rb");
    if (!f) {
        fprintf(stderr, "error: cannot open %s (run: python3 tools/create_hires_demo_dsk.py)\n", disk_path);
        return 1;
    }
    static uint8_t boot_sector[1024];
    size_t got = fread(boot_sector, 1, sizeof(boot_sector), f);
    fclose(f);
    if (got < 256) {
        fprintf(stderr, "error: %s too short to contain a boot sector\n", disk_path);
        return 1;
    }

    apple2_mem_reset();
    reset6502();

    /* Load the boot program directly into $0800, same technique as
     * tests/test_dos33_composed_boot.c -- skips the disk_trap soft-switch
     * protocol since we already have the raw bytes in hand. `got` bytes
     * (not a hardcoded 256) so multi-sector-sized programs (e.g. the
     * checkerboard demo's 460-byte offset-table-driven fill routine)
     * load in full instead of being silently truncated mid-instruction. */
    for (size_t i = 0; i < got; i++) {
        write6502(0x0800 + i, boot_sector[i]);
    }
    pc = 0x0800;

    int is_interactive = isatty(STDIN_FILENO);
    if (is_interactive) {
        enable_raw_mode();
    } else {
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }

    static uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    int frame_count = 0;

    while (1) {
        char c = 0;
        if (read(STDIN_FILENO, &c, 1) == 1 && (c == 'q' || c == 27)) {
            break;
        }

        /* Run enough cycles per frame for the fill loop (8192 bytes at
         * ~11 cycles/iter for STA(zp),Y+INY+BNE, plus the outer page-inc
         * loop and softswitch writes) to complete within 1-2 frames, then
         * keep re-running the halt self-jump harmlessly every frame after.
         * Measured empirically: 20,000 cycles/frame needed ~20 frames to
         * finish; 150,000 finishes within frame 1. */
        exec6502(150000);

        int is_hires = apple2_mem_is_hires_mode();
        int is_page2 = apple2_mem_is_page2_selected();
        int is_mixed = apple2_mem_is_mixed_mode();
        int is_text = apple2_mem_is_text_mode();
        bio_display_render_frame_auto_text_aware(is_hires, is_page2, is_mixed, is_text, read6502, framebuffer);

        if (is_interactive) {
            printf("\x1b[H");
            fb_terminal_viewer_print(framebuffer);
            fflush(stdout);
            usleep(16666);
        }

        frame_count++;
        if (max_frames > 0 && frame_count >= max_frames) {
            break;
        }
    }

    if (!is_interactive) {
        printf("Ran %d frames. HIRES=%d TEXT=%d\n", frame_count,
               apple2_mem_is_hires_mode(), apple2_mem_is_text_mode());
    }

    return 0;
}
