/*
 * main_qemu_zork1boot.c -- QEMU target entry point that boots REAL
 * Zork I (Downloads/Zork_I.dsk) through disk2_controller.c's Disk II
 * emulation and pushes frames to QEMU's live ramfb display.
 */
#include "apple2_mem.h"
#include "cpu6502.h"
#include "bio_display.h"
#include "disk2_controller.h"
#include "zork1_nib_disk_data.h"
#include "uart_keyboard_bridge.h"
#include "emu_trace.h"
#include <stddef.h>

#define UART0_BASE 0x10000000u
static volatile uint8_t *const uart_thr = (volatile uint8_t *)UART0_BASE;
static void qemu_uart_putc(uint8_t byte) {
    *uart_thr = byte;
}

#include "apple2e_system_rom.h"

#define SYSTEM_ROM_SIZE 16384
static uint8_t g_system_rom[SYSTEM_ROM_SIZE];

static void init_system_rom(void) {
    for (int i = 0; i < SYSTEM_ROM_SIZE; i++) {
        g_system_rom[i] = g_apple2e_system_rom[i];
    }
    /* Patch Monitor ROM and BASIC entry points for freestanding boot.
     * Note: 342-0134-a + 342-0135-b is System Monitor ROM only ($F800-$FFFF) with
     * $E000-$E0FF unpopulated (zeros). Vector $FFFE/$FFFF points to $E007.
     * We populate $E000 (cold start), $E003 (warm start), and $E007 (RTI for BRK)
     * so execution lands cleanly at $E000 without a BRK stack-smashing loop. */
    g_system_rom[0x2000] = 0x4C; g_system_rom[0x2001] = 0x00; g_system_rom[0x2002] = 0xE0; /* $E000: JMP $E000 */
    g_system_rom[0x2003] = 0x4C; g_system_rom[0x2004] = 0x00; g_system_rom[0x2005] = 0xE0; /* $E003: JMP $E000 */
    g_system_rom[0x2007] = 0x40;                                                           /* $E007: RTI */
    g_system_rom[0x3F58] = 0x60; /* IORST ($FF58) */
    g_system_rom[0x3E89] = 0x60; /* SETKBD ($FE89) */
    g_system_rom[0x3E93] = 0x60; /* SETVID ($FE93) */
    g_system_rom[0x3B2F] = 0x60; /* INIT ($FB2F) */
    g_system_rom[0x3CA8] = 0xA9; g_system_rom[0x3CA9] = 0x00; g_system_rom[0x3CAA] = 0x60; /* WAIT ($FCA8) */

    /* Real minimal COUT ($FDED, offset $3DED) -- wire character output
     * into screen memory row 0 ($0400), tracking cursor column at zero-page $24. */
    {
        static const uint8_t cout_code[] = {
            0x85, 0xFF,             /* STA $FF */
            0x98,                   /* TYA */
            0x48,                   /* PHA */
            0xA4, 0x24,             /* LDY $24 */
            0xA5, 0xFF,             /* LDA $FF */
            0x99, 0x00, 0x04,       /* STA $0400,Y */
            0xE6, 0x24,             /* INC $24 */
            0x68,                   /* PLA */
            0xA8,                   /* TAY */
            0xA5, 0xFF,             /* LDA $FF */
            0x60                    /* RTS */
        };
        for (size_t i = 0; i < sizeof(cout_code); i++) {
            g_system_rom[0x3DED + i] = cout_code[i];
        }
        g_system_rom[0x388E] = 0x4C; /* JMP $FDED */
        g_system_rom[0x388F] = 0xED;
        g_system_rom[0x3890] = 0xFD;
    }
}

int ramfb_display_init(void);
void ramfb_display_update(const uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]);

static uint16_t g_framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];

static disk2_nibble_track_t g_tracks[DISK2_MAX_TRACKS];

static void load_embedded_nib_disk(void) {
    for (int t = 0; t < G_ZORK1_TRACKS_NUM_TRACKS; t++) {
        for (int b = 0; b < G_ZORK1_TRACKS_MAX_TRACK_BYTES; b++) {
            g_tracks[t].data[b] = g_zork1_tracks_track_data[t][b];
        }
        g_tracks[t].length = g_zork1_tracks_track_lengths[t];
    }
}

#define UART0_LSR 0x10000005u
#define UART0_RBR 0x10000000u

static int qemu_uart_is_ready(void) {
    volatile uint8_t *lsr = (volatile uint8_t *)UART0_LSR;
    return (*lsr & 0x01) != 0;
}

static uint8_t qemu_uart_read_byte(void) {
    volatile uint8_t *rbr = (volatile uint8_t *)UART0_RBR;
    return *rbr;
}

int main(void) {
    emu_trace_init(qemu_uart_putc);
    emu_trace_checkpoint("zork1boot: starting real Zork I boot execution");

    apple2_mem_reset();
    reset6502();

    /* Load patched Apple IIe System ROM ($C000-$FFFF) */
    init_system_rom();
    apple2_mem_load_system_rom(g_system_rom);

    apple2_mem_set_disk_controller_mode(APPLE2_MEM_DISK_CONTROLLER_DISK2);
    disk2_controller_t *ctl = apple2_mem_get_disk2_controller();
    load_embedded_nib_disk();
    disk2_controller_load_nibble_disk(ctl, 0, g_tracks, 0);

    /* Enters at $C600 with slot 6 calling convention (A=X=0x60) */
    pc = 0xC600;
    x = 0x60;
    a = 0x60;

    bio_display_render_frame_auto_text_aware(
        apple2_mem_is_hires_mode(), apple2_mem_is_page2_selected(),
        apple2_mem_is_mixed_mode(), apple2_mem_is_text_mode(),
        read6502, g_framebuffer);
    ramfb_display_update(g_framebuffer);
    int have_ramfb = ramfb_display_init();
    emu_trace_checkpoint(have_ramfb ? "zork1boot: ramfb initialized" : "zork1boot: ramfb missing");

    /* Increased boot cycle budget: 50,000,000 cycles for ZIP interpreter boot */
    uint32_t total_executed = 0;
    const uint32_t cycles_budget = 50000000u;
    const uint32_t chunk = 20000u;
    while (total_executed < cycles_budget) {
        uart_keyboard_bridge_poll(qemu_uart_is_ready, qemu_uart_read_byte);
        exec6502(chunk);
        total_executed += chunk;
        emu_trace_heartbeat(pc, a, x, y, sp, clockticks6502);

        bio_display_render_frame_auto_text_aware(
            apple2_mem_is_hires_mode(), apple2_mem_is_page2_selected(),
            apple2_mem_is_mixed_mode(), apple2_mem_is_text_mode(),
            read6502, g_framebuffer);
        if (have_ramfb) {
            ramfb_display_update(g_framebuffer);
        }
    }

    emu_trace_checkpoint("zork1boot: 50M cycle boot completed, entering interactive loop");

    for (;;) {
        uart_keyboard_bridge_poll(qemu_uart_is_ready, qemu_uart_read_byte);
        exec6502(10000);
        emu_trace_heartbeat(pc, a, x, y, sp, clockticks6502);
        bio_display_render_frame_auto_text_aware(
            apple2_mem_is_hires_mode(), apple2_mem_is_page2_selected(),
            apple2_mem_is_mixed_mode(), apple2_mem_is_text_mode(),
            read6502, g_framebuffer);
        if (have_ramfb) {
            ramfb_display_update(g_framebuffer);
        } else {
            __asm__ volatile("wfi");
        }
    }

    return 0;
}
