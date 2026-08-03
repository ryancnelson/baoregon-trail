/*
 * main_qemu_reinette.c -- QEMU target entry point for the reinette-II-plus
 * port spike (branch spike-reinette-port, NOT the main custom 6502/Disk2
 * emulator). See NEXT_STEPS_REINETTE_SPIKE.md for full context.
 *
 * First real QEMU boot attempt for this spike -- prior work
 * (src/reinette/puce6502_riscv.c/.h, reinette_core.c/.h, reinette_shim.c/.h,
 * reinette_roms.h) has only been standalone host cross-compiled/linked,
 * never actually run under QEMU. This entry point wires it all together:
 * loads the real embedded Apple II+ Autostart ROM + Disk][ boot PROM,
 * resets the CPU, and runs it with a live ramfb display + UART keyboard
 * bridge, matching the existing main_qemu_dos33boot.c/main_qemu_zork1boot.c
 * pattern on `main` as closely as possible so results are comparable.
 *
 * No disk image attached for this first attempt -- real Apple II+ with an
 * empty Disk][ slot 6 either falls through the boot PROM's disk-not-found
 * path into Applesoft's cold-start monitor/BASIC prompt, or hangs in a
 * real disk-search retry loop (both are valid, informative first-boot
 * outcomes; either tells us the CPU/memory core is genuinely executing).
 */
#include "reinette_core.h"
#include "reinette_shim.h"
#include "reinette_roms.h"
#include "bio_display.h"
#include "emu_trace.h"
#include <stddef.h>

/* puce6502_riscv.h redefines uint8_t/uint16_t/bool as plain typedefs
 * (not stdint.h/stdbool.h) to stay freestanding-safe with no libc --
 * matches upstream reinette-II-plus's own puce6502.h exactly. Declare
 * just the two functions this file calls directly, avoiding a full
 * #include that would clash with reinette_core.h's <stdint.h>/
 * <stdbool.h> typedefs in THIS translation unit (reinette_core.c
 * itself never includes puce6502_riscv.h either, for the same reason
 * -- see that file's header comment). */
extern void puce6502RST(void);
extern unsigned short puce6502Exec(unsigned long long cycleCount);

#define UART0_BASE 0x10000000u
static volatile uint8_t *const uart_thr = (volatile uint8_t *)UART0_BASE;
static void qemu_uart_putc(uint8_t byte) {
    *uart_thr = byte;
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

int ramfb_display_init(void);
void ramfb_display_update(const uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]);

static uint16_t g_framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];

static void init_reinette_roms(void) {
    /* reinette_rom[] is REINETTE_ROMSIZE (0x3000 = 12288) bytes,
     * exactly matching g_reinette_appleII_plus_rom's real size. */
    for (unsigned int i = 0; i < REINETTE_ROMSIZE; i++) {
        reinette_rom[i] = g_reinette_appleII_plus_rom[i];
    }
    /* reinette_sl6[] is REINETTE_SL6SIZE (0x100 = 256) bytes, exactly
     * matching g_reinette_diskII_rom's real size (the Disk][ P5A boot
     * PROM real Apple II+ hardware JSRs into at $C600 during cold
     * boot). */
    for (unsigned int i = 0; i < REINETTE_SL6SIZE; i++) {
        reinette_sl6[i] = g_reinette_diskII_rom[i];
    }
}

int main(void) {
    emu_trace_init(qemu_uart_putc);
    emu_trace_checkpoint("reinette: starting first-ever QEMU boot attempt (spike-reinette-port)");

    init_reinette_roms();
    reinette_shim_audio_init();

    /* No disk attached for this first attempt -- see file header
     * comment. reinette_disk_attach() is available if/when a real
     * .nib image is embedded for a follow-up attempt. */

    puce6502RST();
    emu_trace_checkpoint("reinette: puce6502RST() done, entering exec loop");

    bio_display_render_frame_auto_text_aware(
        reinette_HIRES ? 1 : 0, reinette_PAGE2 ? 1 : 0,
        reinette_MIXED ? 1 : 0, reinette_TEXT ? 1 : 0,
        readMem, g_framebuffer);
    ramfb_display_update(g_framebuffer);
    int have_ramfb = ramfb_display_init();
    emu_trace_checkpoint(have_ramfb ? "reinette: ramfb initialized" : "reinette: ramfb missing");

    /* Same 50M-cycle first-boot budget as main_qemu_dos33boot.c/
     * main_qemu_zork1boot.c on `main`, for comparable results. */
    unsigned long long total_executed = 0;
    const unsigned long long cycles_budget = 50000000ull;
    const unsigned long long chunk = 20000ull;
    while (total_executed < cycles_budget) {
        reinette_shim_uart_poll(qemu_uart_is_ready, qemu_uart_read_byte);
        puce6502Exec(chunk);
        total_executed += chunk;
        emu_trace_heartbeat(0, 0, 0, 0, 0, (uint32_t)total_executed);

        reinette_shim_render_frame(g_framebuffer);
        if (have_ramfb) {
            ramfb_display_update(g_framebuffer);
        }
    }

    emu_trace_checkpoint("reinette: 50M cycle first boot attempt completed, entering interactive loop");

    for (;;) {
        reinette_shim_uart_poll(qemu_uart_is_ready, qemu_uart_read_byte);
        puce6502Exec(10000ull);
        reinette_shim_render_frame(g_framebuffer);
        if (have_ramfb) {
            ramfb_display_update(g_framebuffer);
        } else {
            __asm__ volatile("wfi");
        }
    }

    return 0;
}
