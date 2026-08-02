/*
 * main_qemu_disk2boot_zork1.c -- variant of main_qemu_disk2boot.c that
 * boots the REAL, unmodified Zork I disk (~/Downloads/Zork_I.dsk,
 * nibblized via tools/dsk_to_nib.py and embedded via
 * tools/gen_nib_disk_header.py as src/zork1_nib_disk_data.h), instead of
 * the synthetic sample disk.
 *
 * See main_qemu_disk2boot_dos33master.c's header comment for the
 * rationale behind a separate file per disk and the raised cycle
 * budget -- same reasoning applies here. Zork I's real boot loads its
 * own runtime (a Z-machine interpreter) off disk before it can even
 * begin interpreting the actual game's opening text, so likely needs a
 * comparable or larger budget than the DOS 3.3 master boot.
 */
#include "apple2_mem.h"
#include "cpu6502.h"
#include "bio_display.h"
#include "disk2_controller.h"
#include "zork1_nib_disk_data.h"
#include "uart_keyboard_bridge.h"

#define UART0_BASE 0x10000000u
static volatile uint8_t *const uart_thr = (volatile uint8_t *)UART0_BASE;
static void uart_puts(const char *s) {
    while (*s) *uart_thr = (uint8_t)(*s++);
}

int ramfb_display_init(void);
void ramfb_display_update(const uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]);

static uint16_t g_framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];

#define SYSTEM_ROM_SIZE 16384
static uint8_t g_stub_rom[SYSTEM_ROM_SIZE];

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

    uart_puts("disk2boot-zork1: starting boot execution\n");

    bio_display_render_frame_auto_text_aware(
        apple2_mem_is_hires_mode(), apple2_mem_is_page2_selected(),
        apple2_mem_is_mixed_mode(), apple2_mem_is_text_mode(),
        read6502, g_framebuffer);
    ramfb_display_update(g_framebuffer);
    int have_ramfb = ramfb_display_init();
    uart_puts(have_ramfb ? "have_ramfb=1\n" : "have_ramfb=0\n");

    uint32_t total_executed = 0;
    const uint32_t cycles_budget = 60000000u;
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

    uart_puts("disk2boot-zork1: boot execution budget exhausted, entering interactive loop\n");

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
