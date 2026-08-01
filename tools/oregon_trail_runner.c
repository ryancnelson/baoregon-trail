/*
 * tools/oregon_trail_runner.c -- Loads the two-segment Oregon Trail title
 * screen program (tools/oregon_trail_title.bin at $0800, tools/
 * oregon_trail_title_data.bin at $4000) into the emulator at their real,
 * non-overlapping addresses and renders the result live in the terminal.
 *
 * Unlike hires_demo_runner.c (single-blob-at-$0800 loader), this program
 * needs two separate load addresses because the bitmap data must live
 * outside the $2000-$3FFF Hi-Res destination range it's being copied
 * into -- putting it at $0800+ (right after the code) caused the copy
 * loop to read from memory it was simultaneously overwriting once the
 * copy progressed far enough to reach $2000, corrupting the back half of
 * the image (confirmed via direct emulator-memory-vs-source-file diff:
 * 530/8192 bytes differed, starting exactly where the source pointer
 * caught up to $2000).
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

static size_t load_file_at(const char *path, uint16_t addr) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "error: cannot open %s\n", path);
        exit(1);
    }
    static uint8_t buf[16384];
    size_t got = fread(buf, 1, sizeof(buf), f);
    fclose(f);
    for (size_t i = 0; i < got; i++) {
        write6502((uint16_t)(addr + i), buf[i]);
    }
    return got;
}

int main(int argc, char **argv) {
    const char *code_path = (argc > 1) ? argv[1] : "tools/oregon_trail_title.bin";
    const char *data_path = (argc > 2) ? argv[2] : "tools/oregon_trail_title_data.bin";
    int max_frames = (argc > 3) ? atoi(argv[3]) : 0;

    apple2_mem_reset();
    reset6502();

    size_t code_len = load_file_at(code_path, 0x0800);
    size_t data_len = load_file_at(data_path, 0x4000);
    fprintf(stderr, "Loaded code (%zu bytes @ $0800) + data (%zu bytes @ $4000)\n",
            code_len, data_len);
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
    uint16_t last_pc = pc;
    int stuck_frames = 0;

    while (1) {
        char c = 0;
        if (read(STDIN_FILENO, &c, 1) == 1 && (c == 'q' || c == 27)) {
            break;
        }

        uint16_t pc_before = pc;
        exec6502(400000);

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
            printf("\x1b[2J\x1b[H");
            fb_terminal_viewer_print(framebuffer);
            printf("PC=$%04X HIRES=%d TEXT=%d frame=%d stuck=%d\n",
                   last_pc, is_hires, is_text, frame_count, stuck_frames);
            fflush(stdout);
            usleep(16666);
        }

        frame_count++;
        if (max_frames > 0 && frame_count >= max_frames) {
            break;
        }
        if (stuck_frames > 5 && !is_interactive) {
            break;
        }
    }

    if (!is_interactive) {
        printf("Ran %d frames. Final PC=$%04X HIRES=%d TEXT=%d stuck_frames=%d\n",
               frame_count, last_pc, apple2_mem_is_hires_mode(), apple2_mem_is_text_mode(), stuck_frames);
    }

    return 0;
}
