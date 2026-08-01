#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/bio_display.h"

/* Renders a raw 64KB g_ram[] dump (e.g. pulled via QEMU's monitor
 * `pmemsave <g_ram addr> 0x10000 <file>`, or MAME's Lua dump scripts)
 * through the real, tested video decode pipeline -- host-side, since
 * neither QEMU nor MAME's headless mode has an actual display attached.
 *
 * Usage: render_qemu_dump <ram_dump.bin> <out_framebuffer.raw>
 */
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <ram_dump.bin> <out_framebuffer.raw>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "error: cannot open %s\n", argv[1]);
        return 1;
    }
    static uint8_t ram[65536];
    size_t got = fread(ram, 1, sizeof(ram), f);
    fclose(f);
    if (got != sizeof(ram)) {
        fprintf(stderr, "error: %s is %zu bytes, expected 65536\n", argv[1], got);
        return 1;
    }

    apple2_mem_reset();
    for (int i = 0; i < 8192; i++) {
        write6502((uint16_t)(0x2000 + i), ram[0x2000 + i]);
    }
    write6502(0xC057, 0x00); /* HIRES on */
    write6502(0xC052, 0x00); /* MIXED off (full-screen) */
    write6502(0xC050, 0x00); /* GRAPHICS on (TEXT off) */
    write6502(0xC054, 0x00); /* PAGE2 off (page 1) */

    static uint16_t fb[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    bio_display_render_frame_auto_text_aware(
        apple2_mem_is_hires_mode(), apple2_mem_is_page2_selected(),
        apple2_mem_is_mixed_mode(), apple2_mem_is_text_mode(), read6502, fb);

    FILE *out = fopen(argv[2], "wb");
    if (!out) {
        fprintf(stderr, "error: cannot write %s\n", argv[2]);
        return 1;
    }
    fwrite(fb, sizeof(uint16_t), BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT, out);
    fclose(out);
    printf("Wrote %s\n", argv[2]);
    return 0;
}
