/*
 * main_qemu_dos33boot.c -- QEMU target entry point that boots the REAL
 * Apple DOS 3.3 Master Disk (Downloads/Apple_DOS_3.3_Master.dsk) through
 * disk2_controller.c's Disk II emulation and pushes frames to QEMU's live
 * ramfb display.
 */
#include "apple2_mem.h"
#include "cpu6502.h"
#include "bio_display.h"
#include "disk2_controller.h"
#include "dos33_master_nib_disk_data.h"
#include "uart_keyboard_bridge.h"
#include "emu_trace.h"

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
    /* Patch Monitor ROM and BASIC entry points for freestanding boot */
    g_system_rom[0x2000] = 0x4C; g_system_rom[0x2001] = 0x00; g_system_rom[0x2002] = 0xE0; /* JMP $E000 (Applesoft BASIC spin loop) */
    g_system_rom[0x3F58] = 0x60; /* IORST ($FF58) */
    g_system_rom[0x3E89] = 0x60; /* SETKBD ($FE89) */
    g_system_rom[0x3E93] = 0x60; /* SETVID ($FE93) */
    g_system_rom[0x3B2F] = 0x60; /* INIT ($FB2F) */
    g_system_rom[0x388E] = 0x60; /* COUT ($F88E) */
    g_system_rom[0x3CA8] = 0xA9; g_system_rom[0x3CA9] = 0x00; g_system_rom[0x3CAA] = 0x60; /* WAIT ($FCA8) */
}

int ramfb_display_init(void);
void ramfb_display_update(const uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]);

static uint16_t g_framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];

static disk2_nibble_track_t g_tracks[DISK2_MAX_TRACKS];

static void load_embedded_nib_disk(void) {
    for (int t = 0; t < G_DOS33_MASTER_TRACKS_NUM_TRACKS; t++) {
        for (int b = 0; b < G_DOS33_MASTER_TRACKS_MAX_TRACK_BYTES; b++) {
            g_tracks[t].data[b] = g_dos33_master_tracks_track_data[t][b];
        }
        g_tracks[t].length = g_dos33_master_tracks_track_lengths[t];
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
    emu_trace_checkpoint("dos33boot: starting real Apple DOS 3.3 Master boot execution");

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
    emu_trace_checkpoint(have_ramfb ? "dos33boot: ramfb initialized" : "dos33boot: ramfb missing");

    /* Increased boot cycle budget: 50,000,000 cycles for multi-sector DOS boot */
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

    emu_trace_checkpoint("dos33boot: 50M cycle boot completed, entering interactive loop");

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
