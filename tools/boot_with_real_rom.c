/*
 * tools/boot_with_real_rom.c -- Boots a real DOS 3.3 disk through
 * baoregon-trail's OWN composed emulator (cpu6502 + apple2_mem +
 * disk_trap), now with the real Apple IIe system ROM loaded at
 * $C000-$FFFF via apple2_mem_load_system_rom() -- unlike
 * boot_real_disk.c, this should get past the JMP-into-peripheral-ROM
 * boundary found earlier, since real ROM code now actually exists there.
 *
 * Usage: boot_with_real_rom <rom_c000_ffff.bin> <dsk_path> [cycles_per_frame] [max_frames]
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <stdint.h>

#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/disk_trap.h"
#include "../src/bio_display.h"

void fb_terminal_viewer_print(const uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]);

#define DSK_SIZE 143360
#define ROM_SIZE 16384

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
static uint8_t g_rom_image[ROM_SIZE];

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <rom_c000_ffff.bin> <dsk_path> [cycles_per_frame] [max_frames]\n", argv[0]);
        return 1;
    }
    const char *rom_path = argv[1];
    const char *dsk_path = argv[2];
    uint32_t cycles_per_frame = (argc > 3) ? (uint32_t)atol(argv[3]) : 150000;
    int max_frames = (argc > 4) ? atoi(argv[4]) : 0;

    FILE *rf = fopen(rom_path, "rb");
    if (!rf) { fprintf(stderr, "error: cannot open ROM %s\n", rom_path); return 1; }
    size_t rgot = fread(g_rom_image, 1, ROM_SIZE, rf);
    fclose(rf);
    if (rgot != ROM_SIZE) {
        fprintf(stderr, "error: ROM %s is %zu bytes, expected %d\n", rom_path, rgot, ROM_SIZE);
        return 1;
    }

    FILE *df = fopen(dsk_path, "rb");
    if (!df) { fprintf(stderr, "error: cannot open disk %s\n", dsk_path); return 1; }
    size_t dgot = fread(g_disk_image, 1, DSK_SIZE, df);
    fclose(df);
    if (dgot != DSK_SIZE) {
        fprintf(stderr, "error: disk %s is %zu bytes, expected %d\n", dsk_path, dgot, DSK_SIZE);
        return 1;
    }

    apple2_mem_reset();
    reset6502();
    apple2_mem_load_system_rom(g_rom_image);

    /* Real boot path: register the disk image, drive the real $C0E0-$C0EF
     * protocol to load Track 0/Sector 0, then set PC from the REAL 6502
     * reset vector ($FFFC/$FFFD) -- now backed by real ROM data -- instead
     * of hardcoding $0800. This is a more faithful boot than
     * boot_real_disk.c: real hardware's reset vector points into ROM
     * (which then reads the boot sector itself via the disk-controller
     * card), not directly at $0800. */
    disk_trap_set_image(g_disk_image);
    write6502(0xC0E0, 0x00);
    write6502(0xC0E1, 0x00);
    for (int i = 0; i < 256; i++) {
        uint8_t b = read6502(0xC0EC);
        write6502(0x0800 + i, b);
    }

    uint16_t reset_vec = (uint16_t)read6502(0xFFFC) | ((uint16_t)read6502(0xFFFD) << 8);
    fprintf(stderr, "Loaded ROM (%zu bytes) + disk %s (%zu bytes). Real reset vector = $%04X\n",
            rgot, dsk_path, dgot, reset_vec);
    /* Real Apple II boot: PC starts at the ROM reset vector, NOT $0800 --
     * the ROM's own autostart code is what reads the boot sector via the
     * disk-controller card in the first place, on real hardware. We've
     * already pre-loaded $0800 above as a shortcut/fallback; starting PC
     * at the real reset vector tests whether the real ROM code path
     * itself can drive our disk_trap correctly. */
    pc = reset_vec;

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
            break;
        }
    }

    if (!is_interactive) {
        printf("Ran %d frames. Final PC=$%04X HIRES=%d TEXT=%d stuck_frames=%d\n",
               frame_count, last_pc, apple2_mem_is_hires_mode(), apple2_mem_is_text_mode(), stuck_frames);
    }

    return 0;
}
