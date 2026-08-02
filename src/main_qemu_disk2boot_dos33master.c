/*
 * main_qemu_disk2boot_dos33master.c -- variant of main_qemu_disk2boot.c
 * that boots the REAL, unmodified Apple DOS 3.3 Master disk
 * (~/Downloads/Apple_DOS_3.3_Master.dsk, nibblized via tools/dsk_to_nib.py
 * and embedded via tools/gen_nib_disk_header.py as
 * src/dos33_master_nib_disk_data.h), instead of the synthetic
 * one-line-then-spin sample disk main_qemu_disk2boot.c uses.
 *
 * WHY A SEPARATE FILE instead of a runtime flag: the embedded nibble
 * track data is a compile-time constant array (~1MB header) selected via
 * #include, matching this project's existing precedent (each demo/tool
 * that embeds a specific disk image gets its own small entry-point file
 * rather than one entry point with a runtime disk-select branch over
 * multiple giant embedded arrays).
 *
 * CYCLE BUDGET: raised from the synthetic sample's 5,000,000 to
 * 60,000,000 for this real disk. A real Apple II DOS 3.3 boot to a
 * stable ']' prompt does substantially more work than the sample's
 * single write-a-string-then-spin-forever design: reading and
 * interpreting the boot loader, reading the actual DOS image off
 * multiple tracks/sectors, initializing DOS's own RWTS/FM routines,
 * relocating itself into high memory, and finally presenting the
 * "]" prompt. NOTE: this program's own post-budget "interactive loop"
 * (see main_qemu_disk2boot.c) keeps calling exec6502() forever regardless
 * of this budget value -- the budget mainly controls the UART log-line
 * timing and chunk size, not whether the boot can progress past it. The
 * real requirement for reaching a stable prompt is enough real
 * wall-clock time for the emulated CPU to get there, not a specific
 * cycle-budget cutoff.
 */
#include "apple2_mem.h"
#include "cpu6502.h"
#include "bio_display.h"
#include "disk2_controller.h"
#include "dos33_master_nib_disk_data.h"
#include "uart_keyboard_bridge.h"

#define UART0_BASE 0x10000000u
static volatile uint8_t *const uart_thr = (volatile uint8_t *)UART0_BASE;
static void uart_puts(const char *s) {
    while (*s) *uart_thr = (uint8_t)(*s++);
}

int ramfb_display_init(void);
void ramfb_display_update(const uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]);

static uint16_t g_framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];

/* Minimal stub system ROM: every byte is 0x60 (RTS) so any JSR into ROM
 * territory returns immediately, except $FCA8 (real Apple II Monitor
 * ROM's WAIT subroutine), patched to `LDA #$00; RTS` -- matches
 * boot_disk2_real_dsk_stubrom.c's proven-working stub exactly. */
#define SYSTEM_ROM_SIZE 16384
static uint8_t g_stub_rom[SYSTEM_ROM_SIZE];

/* disk2_nibble_track_t is {uint8_t data[DISK2_MAX_TRACK_BYTES]; int length;}
 * -- build the array disk2_controller_load_nibble_disk() expects from the
 * embedded flat tables (dos33_master_nib_disk_data.h can't emit the
 * struct type directly since disk2_controller.h isn't visible to the
 * standalone gen_nib_disk_header.py script). */
static disk2_nibble_track_t g_tracks[DISK2_MAX_TRACKS];

static void load_embedded_nib_disk(void) {
    for (int t = 0; t < DOS33_MASTER_NUM_TRACKS; t++) {
        for (int b = 0; b < DOS33_MASTER_MAX_TRACK_BYTES; b++) {
            g_tracks[t].data[b] = dos33_master_track_data[t][b];
        }
        g_tracks[t].length = dos33_master_track_lengths[t];
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
    apple2_mem_reset();
    reset6502();

    for (int i = 0; i < SYSTEM_ROM_SIZE; i++) g_stub_rom[i] = 0x60;
    g_stub_rom[0x3CA8] = 0xA9; /* LDA #$00 */
    g_stub_rom[0x3CA9] = 0x00;
    g_stub_rom[0x3CAA] = 0x60; /* RTS */
    apple2_mem_load_system_rom(g_stub_rom);

    apple2_mem_set_disk_controller_mode(APPLE2_MEM_DISK_CONTROLLER_DISK2);
    disk2_controller_t *ctl = apple2_mem_get_disk2_controller();
    load_embedded_nib_disk();
    disk2_controller_load_nibble_disk(ctl, 0, g_tracks, 0);

    pc = 0xC600;
    x = 0x60;
    a = 0x60;

    uart_puts("disk2boot-dos33master: starting boot execution\n");

    bio_display_render_frame_auto_text_aware(
        apple2_mem_is_hires_mode(), apple2_mem_is_page2_selected(),
        apple2_mem_is_mixed_mode(), apple2_mem_is_text_mode(),
        read6502, g_framebuffer);
    ramfb_display_update(g_framebuffer);
    int have_ramfb = ramfb_display_init();
    uart_puts(have_ramfb ? "have_ramfb=1\n" : "have_ramfb=0\n");

    uint32_t total_executed = 0;
    const uint32_t cycles_budget = 60000000u; /* raised for a real DOS 3.3 boot */
    const uint32_t chunk = 20000u;
    while (total_executed < cycles_budget) {
        uart_keyboard_bridge_poll(qemu_uart_is_ready, qemu_uart_read_byte);
        exec6502(chunk);
        total_executed += chunk;

        bio_display_render_frame_auto_text_aware(
            apple2_mem_is_hires_mode(), apple2_mem_is_page2_selected(),
            apple2_mem_is_mixed_mode(), apple2_mem_is_text_mode(),
            read6502, g_framebuffer);
        if (have_ramfb) {
            ramfb_display_update(g_framebuffer);
        }
    }

    uart_puts("disk2boot-dos33master: boot execution budget exhausted, entering interactive loop\n");

    for (;;) {
        uart_keyboard_bridge_poll(qemu_uart_is_ready, qemu_uart_read_byte);
        exec6502(10000);
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
