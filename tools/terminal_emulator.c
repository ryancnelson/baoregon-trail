/*
 * tools/terminal_emulator.c -- Host-only interactive terminal runner for
 * Bao-Oregon-Trail Apple II emulator (NEXT_STEPS.md Step 4).
 *
 * Runs the composed system end-to-end:
 *   - Initializes emulator_loop (splash menu -> multi-game selector).
 *   - Reads non-blocking key input from stdin (mapped to button 0/1/2 or 6502 keys).
 *   - Executes one frame (baoregon_emulator_run_frame()).
 *   - Dumps and renders the 320x240 RGB565 framebuffer directly in the terminal
 *     using ANSI 24-bit truecolor half-blocks (U+2580).
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <stdint.h>

#include "../src/emulator_loop.h"
#include "../src/apple2_mem.h"
#include "../src/bio_display.h"

void fb_terminal_viewer_print(const uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]);

static struct termios orig_termios;

static void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf("\x1b[0m\x1b[?25h\n"); /* reset ANSI colors & show cursor */
}

static void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    /* Hide cursor and clear terminal screen */
    printf("\x1b[?25l\x1b[2J\x1b[H");
}

int main(int argc, char **argv) {
    int max_frames = 0;
    if (argc > 1) {
        max_frames = atoi(argv[1]);
    }

    /* Initialize emulator loop */
    baoregon_emulator_init();

    int is_interactive = isatty(STDIN_FILENO);
    if (is_interactive) {
        enable_raw_mode();
    } else {
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    }

    int frame_count = 0;
    while (1) {
        /* Process input */
        char c = 0;
        if (read(STDIN_FILENO, &c, 1) == 1) {
            if (c == 'q' || c == 27 /* ESC */) {
                break;
            }
            if (c == 'w' || c == 'k' || c == 'A') { /* UP / PREV -> Button 0 */
                apple2_mem_set_button_state(0, 1);
            } else if (c == 's' || c == 'j' || c == 'B') { /* DOWN / NEXT -> Button 1 */
                apple2_mem_set_button_state(1, 1);
            } else if (c == ' ' || c == '\n' || c == '\r') { /* SELECT -> Button 2 */
                apple2_mem_set_button_state(2, 1);
            } else {
                apple2_mem_inject_key((uint8_t)c);
            }
        }

        /* Run one frame */
        baoregon_emulator_poll_input();
        baoregon_emulator_run_frame();

        /* Clear button press state after polling */
        apple2_mem_set_button_state(0, 0);
        apple2_mem_set_button_state(1, 0);
        apple2_mem_set_button_state(2, 0);

        /* Render framebuffer to terminal if interactive or requested */
        if (is_interactive) {
            printf("\x1b[H"); /* Home cursor */
            fb_terminal_viewer_print(baoregon_emulator_get_framebuffer());
            fflush(stdout);
            usleep(16666); /* ~60 FPS delay */
        }

        frame_count++;
        if (max_frames > 0 && frame_count >= max_frames) {
            break;
        }
    }

    if (!is_interactive) {
        printf("Ran %d frames in non-interactive batch mode successfully.\n", frame_count);
    }

    return 0;
}
