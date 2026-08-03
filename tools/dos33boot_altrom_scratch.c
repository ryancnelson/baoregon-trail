/*
 * tools/dos33boot_altrom_scratch.c -- SCRATCH parallel test entry point,
 * NOT src/main_qemu_dos33boot.c (that file is untouched). Boots the real
 * DOS 3.3 Master disk against apple2-asoft-auto.rom (real Apple II+
 * Autostart ROM with genuine Applesoft BASIC, verified authentic via
 * tools/test_alt_rom_scratch.py) instead of the project's real
 * Monitor-only src/apple2e_system_rom.h, under QEMU with a live ramfb
 * display -- to get real, visual confirmation (per Ryan's task) that
 * this alt ROM resolves the DOS 3.3 banner-text gap.
 *
 * Host-side test (same disk data + same 6502 core, /tmp scratch
 * harness, not committed) already confirmed a REAL PASS: screen memory
 * shows "DOS VERSION 3.3   08/25/80" / "APPLE II PLUS OR ROMCARD
 * SYSTEM MASTER" / "(LOADING INTEGER INTO LANGUAGE CARD)". This file
 * reproduces that exact same result under real QEMU with a live window
 * for visual/screenshot proof.
 */
#include "apple2_mem.h"
#include "cpu6502.h"
#include "bio_display.h"
#include "disk2_controller.h"
#include "dos33_master_nib_disk_data.h"
#include "uart_keyboard_bridge.h"
#include "emu_trace.h"
#include "alt_rom_asoft_auto.h"
#include <stddef.h>

#define UART0_BASE 0x10000000u
static volatile uint8_t *const uart_thr = (volatile uint8_t *)UART0_BASE;
static void qemu_uart_putc(uint8_t byte) {
    *uart_thr = byte;
}

#define SYSTEM_ROM_SIZE 16384
static uint8_t g_system_rom[SYSTEM_ROM_SIZE];

static void init_system_rom(void) {
    /* Use the alt ROM AS-IS -- real Apple II+ Autostart ROM with its own
     * working RESET vector ($FA62, confirmed via direct byte inspection
     * to be genuine 6502 code: CLD/JSR $FE84/JSR $FB2F/JSR $FE93/
     * JSR $FE89/LDA $C058...) and real Applesoft BASIC. Does NOT need
     * any of main_qemu_dos33boot.c's Monitor-only-ROM patches (no COUT
     * stub, no E000/E003/E007 landing pads, no WAIT/SETKBD/SETVID/INIT
     * stubs) -- this ROM has genuine working code at all those
     * addresses already. */
    for (int i = 0; i < SYSTEM_ROM_SIZE; i++) {
        g_system_rom[i] = g_alt_rom_asoft_auto[i];
    }
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
    emu_trace_checkpoint("dos33boot-altrom: starting DOS 3.3 Master boot with real Applesoft ROM (apple2-asoft-auto.rom)");

    apple2_mem_reset();
    reset6502();

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
    emu_trace_checkpoint(have_ramfb ? "dos33boot-altrom: ramfb initialized" : "dos33boot-altrom: ramfb missing");

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

    emu_trace_checkpoint("dos33boot-altrom: 50M cycle boot completed, entering interactive loop");

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
