/*
 * main_renode_demo.c -- minimum-viable Renode demo entry point for the
 * REAL Baochip-1x target (linker.ld's real, corrected addresses:
 * ReRAM @ 0x60000000, SRAM @ 0x61000000 -- see
 * docs/baochip-1x-memory-map-findings.md).
 *
 * Distinct from main.c (the real silent bare-metal target) and the
 * main_qemu_*.c family (QEMU 'virt' machine, different memory map/UART
 * MMIO address entirely) -- this is specifically for exercising the
 * real Baochip-1x address layout under Renode's platform emulation
 * (renode/bao1x.repl), via the real DUART peripheral
 * (0x40042000, SFR_TXD @ offset 0x0 -- see renode/duart_console.repl
 * and bao1x_peri.svd) for visible console output, matching this
 * project's baoregon-trail issue #2 MVP-demo requirement: real,
 * visible output on the new Renode rig, not just scoping.
 *
 * Runs the real 6502/Apple II emulator loop underneath (same
 * baoregon_emulator_* API main.c uses) so this is a genuine exercise
 * of the actual emulator core against the real corrected memory map,
 * not a bare "hello world" toy -- periodically reports a heartbeat
 * line with the real emulator's total_cycles counter to DUART so the
 * demo's liveness is directly observable.
 */
#include "emulator_loop.h"
#include "boot_perf.h"

#define DUART_BASE 0x40042000u
#define DUART_TXD  (*(volatile uint32_t *)(DUART_BASE + 0x0000u))

static void duart_putc(char c) {
    DUART_TXD = (uint32_t)(unsigned char)c;
}

static void duart_puts(const char *s) {
    while (*s) {
        duart_putc(*s++);
    }
}

static void duart_put_hex32(uint32_t v) {
    static const char hex[] = "0123456789ABCDEF";
    duart_puts("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        duart_putc(hex[(v >> shift) & 0xF]);
    }
}

boot_perf_metrics_t g_boot_metrics;

int main(void) {
    duart_puts("\r\n");
    duart_puts("baoregon-trail: real Baochip-1x Renode demo (issue #2)\r\n");
    duart_puts("linker.ld real addresses: ReRAM=0x60000000 SRAM=0x61000000\r\n");
    duart_puts("Booting real emulator_loop...\r\n");

    boot_perf_init(&g_boot_metrics);
    boot_perf_record_boot(&g_boot_metrics, 0ULL);
    baoregon_emulator_init();

    duart_puts("emulator_init() done, running real frames...\r\n");

    uint32_t frame = 0;
    for (;;) {
        baoregon_emulator_run_frame();
        frame++;
        if ((frame & 0xFFu) == 0u) {
            duart_puts("heartbeat: frame=");
            duart_put_hex32(frame);
            duart_puts(" total_cycles=");
            duart_put_hex32((uint32_t)baoregon_emulator_get_total_cycles());
            duart_puts("\r\n");
        }
    }

    return 0;
}
