/*
 * main.c -- Main entry point for bare-metal RISC-V Baochip-1x execution.
 */
#include "emulator_loop.h"
#include "boot_perf.h"

boot_perf_metrics_t g_boot_metrics;

void baoregon_main_init(uint64_t boot_cycles) {
    boot_perf_init(&g_boot_metrics);
    boot_perf_record_boot(&g_boot_metrics, boot_cycles);
    baoregon_emulator_init();
}

const boot_perf_metrics_t *baoregon_get_boot_metrics(void) {
    return &g_boot_metrics;
}

#ifndef TEST_BUILD
int main(void) {
    baoregon_main_init(0ULL);

    for (;;) {
        baoregon_emulator_run_frame();
    }

    return 0;
}
#endif
