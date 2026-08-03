/*
 * main_qemu_loderunner.c -- QEMU target entry point that boots REAL
 * Lode Runner (4am-preservationist crack, tools/loderunner_4amcrack.dsk)
 * through disk2_controller.c's Disk II emulation and pushes frames to
 * QEMU's live ramfb display.
 *
 * Unlike main_qemu_dos33boot.c/main_qemu_zork1boot.c, this target uses
 * src/apple2_autostart_rom.h (the real Apple II+ Autostart ROM) rather
 * than src/apple2e_system_rom.h (the real Apple IIe Monitor-only ROM)
 * -- Lode Runner's own boot-sector code makes a real JMP (indirect)
 * through a low-memory vector that the IIe ROM's own reset/autostart
 * sequence never initializes (confirmed via host-side tracing: the
 * IIe ROM crashes to PC=$0000 within ~2300 cycles, well before any
 * disk read even happens), while the II+ Autostart ROM handles this
 * correctly and boots all the way to real HGR gameplay graphics
 * (independently confirmed via vision_analyze on the decoded HGR
 * framebuffer -- ladders, platforms, character sprites, without being
 * told the game -- see NEXT_STEPS.md).
 *
 * Real Apple II+ hardware has no ROM at $C000-$CFFF (that's peripheral
 * card I/O space), so this ROM needs no COUT/monitor-entry-point
 * patches at all -- it's used verbatim, matching
 * tools/loderunner_altrom_boot.c's originally-confirmed-working
 * approach.
 */
#include "apple2_mem.h"
#include "cpu6502.h"
#include "bio_display.h"
#include "disk2_controller.h"
#include "loderunner_nib_disk_data.h"
#include "uart_keyboard_bridge.h"
#include "emu_trace.h"
#include <stddef.h>

#define UART0_BASE 0x10000000u
static volatile uint8_t *const uart_thr = (volatile uint8_t *)UART0_BASE;
static void qemu_uart_putc(uint8_t byte) {
    *uart_thr = byte;
}

#include "apple2_autostart_rom.h"

int ramfb_display_init(void);
void ramfb_display_update(const uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]);

static uint16_t g_framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];

static disk2_nibble_track_t g_tracks[DISK2_MAX_TRACKS];

static void load_embedded_nib_disk(void) {
    for (int t = 0; t < G_LODERUNNER_TRACKS_NUM_TRACKS; t++) {
        for (int b = 0; b < G_LODERUNNER_TRACKS_MAX_TRACK_BYTES; b++) {
            g_tracks[t].data[b] = g_loderunner_tracks_track_data[t][b];
        }
        g_tracks[t].length = g_loderunner_tracks_track_lengths[t];
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
    emu_trace_checkpoint("loderunner: starting real Lode Runner boot execution");

    apple2_mem_reset();
    reset6502();

    /* Load the real Apple II+ Autostart ROM ($C000-$FFFF, $C000-$CFFF
     * zero-filled -- real II+ hardware has peripheral card I/O there,
     * not ROM) verbatim, no patches needed. */
    apple2_mem_load_system_rom(g_apple2_autostart_rom);

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
    emu_trace_checkpoint(have_ramfb ? "loderunner: ramfb initialized" : "loderunner: ramfb missing");

    /* Real host-side confirmation (tools/loderunner_hgr_dump.c) reached
     * genuine HGR gameplay graphics by ~1,000,000,000 cycles; use the
     * same budget here before switching to the steady-state interactive
     * loop. */
    uint32_t total_executed = 0;
    const uint32_t cycles_budget = 1000000000u;
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

    emu_trace_checkpoint("loderunner: 1B cycle boot completed, entering interactive loop");

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
