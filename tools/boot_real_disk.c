/*
 * tools/boot_real_disk.c -- Boots a REAL Apple II .dsk image (e.g. a real
 * Zork I / Infocom disk, not a synthetic hand-assembled bootloader) through
 * the actual disk_trap soft-switch protocol, same technique as
 * tests/test_dos33_composed_boot.c but continuously rendered and with a
 * configurable cycle budget so we can watch how far real disk content
 * actually gets on the composed system.
 *
 * Usage: boot_real_disk <dsk_path> [cycles_per_frame] [max_frames]
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>

#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/disk_trap.h"
#include "../src/bio_display.h"

void fb_terminal_viewer_print(const uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]);

#define DSK_SIZE 143360

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

static uint8_t g_disk_image[DSK_SIZE];

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <dsk_path> [cycles_per_frame] [max_frames]\n", argv[0]);
        return 1;
    }
    const char *dsk_path = argv[1];
    uint32_t cycles_per_frame = (argc > 2) ? (uint32_t)atol(argv[2]) : 150000;
    int max_frames = (argc > 3) ? atoi(argv[3]) : 0;

    FILE *f = fopen(dsk_path, "rb");
    if (!f) {
        fprintf(stderr, "error: cannot open %s\n", dsk_path);
        return 1;
    }
    size_t got = fread(g_disk_image, 1, DSK_SIZE, f);
    fclose(f);
    if (got != DSK_SIZE) {
        fprintf(stderr, "error: %s is %zu bytes, expected %d\n", dsk_path, got, DSK_SIZE);
        return 1;
    }

    apple2_mem_reset();
    reset6502();

    /* Real boot path: register the disk image with the trap, then drive
     * the SAME $C0E0-$C0EF soft-switch protocol real DOS/RWTS would use
     * to read Track 0 Sector 0 into $0800 -- mirrors
     * tests/test_dos33_composed_boot.c exactly, just with real content. */
    disk_trap_set_image(g_disk_image);
    write6502(0xC0E0, 0x00); /* select track 0 */
    write6502(0xC0E1, 0x00); /* select sector 0 */
    for (int i = 0; i < 256; i++) {
        uint8_t b = read6502(0xC0EC);
        write6502(0x0800 + i, b);
    }
    pc = 0x0800;

    fprintf(stderr, "Loaded %s (%zu bytes). Boot sector[0..7] = %02X %02X %02X %02X %02X %02X %02X %02X\n",
            dsk_path, got, g_disk_image[0], g_disk_image[1], g_disk_image[2], g_disk_image[3],
            g_disk_image[4], g_disk_image[5], g_disk_image[6], g_disk_image[7]);

    int is_interactive = isatty(STDIN_FILENO);
    if (is_interactive) {
        enable_raw_mode();
    } else {
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }

    static uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    int frame_count = 0;
    uint16_t last_pc = pc;
    int stuck_frames = 0;

    while (1) {
        char c = 0;
        if (read(STDIN_FILENO, &c, 1) == 1 && (c == 'q' || c == 27)) {
            break;
        }

        uint16_t pc_before = pc;
        exec6502(cycles_per_frame);

        if (pc == pc_before) {
            stuck_frames++;
        } else {
            stuck_frames = 0;
        }
        last_pc = pc;

        int is_hires = apple2_mem_is_hires_mode();
        int is_page2 = apple2_mem_is_page2_selected();
        int is_mixed = apple2_mem_is_mixed_mode();
        int is_text = apple2_mem_is_text_mode();
        bio_display_render_frame_auto_text_aware(is_hires, is_page2, is_mixed, is_text, read6502, framebuffer);

        if (is_interactive) {
            printf("\x1b[H");
            fb_terminal_viewer_print(framebuffer);
            printf("PC=$%04X HIRES=%d TEXT=%d MIXED=%d PAGE2=%d frame=%d stuck=%d\n",
                   last_pc, is_hires, is_text, is_mixed, is_page2, frame_count, stuck_frames);
            fflush(stdout);
            usleep(16666);
        }

        frame_count++;
        if (max_frames > 0 && frame_count >= max_frames) {
            break;
        }
        if (stuck_frames > 5 && !is_interactive) {
            break; /* batch mode: stop once PC has parked (self-jump trap or halt) */
        }
    }

    if (!is_interactive) {
        printf("Ran %d frames. Final PC=$%04X HIRES=%d TEXT=%d stuck_frames=%d\n",
               frame_count, last_pc, apple2_mem_is_hires_mode(), apple2_mem_is_text_mode(), stuck_frames);
    }

    return 0;
}
