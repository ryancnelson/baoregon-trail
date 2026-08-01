/*
 * boot_perf.c -- Boot timing and performance benchmark implementation for Baochip-1x.
 */
#include "boot_perf.h"

void boot_perf_init(boot_perf_metrics_t *metrics) {
    if (!metrics) return;
    metrics->boot_cycles = 0;
    metrics->boot_time_us = 0;
    metrics->is_sub_50ms = 0;
}

void boot_perf_record_boot(boot_perf_metrics_t *metrics, uint64_t cycles) {
    if (!metrics) return;
    metrics->boot_cycles = cycles;

    /* 350 MHz main CPU clock rate = 350 cycles per microsecond.
     * Check high 32 bits of 64-bit cycle count to prevent truncation overflow. */
    if ((cycles >> 32) != 0 || cycles > BAOCHIP_MAX_BOOT_CYCLES) {
        metrics->boot_time_us = BAOCHIP_MAX_BOOT_TIME_US + 1u;
        metrics->is_sub_50ms = 0;
        return;
    }

    uint32_t cycles32 = (uint32_t)cycles;
    metrics->boot_time_us = cycles32 / 350u;
    metrics->is_sub_50ms = (metrics->boot_time_us < BAOCHIP_MAX_BOOT_TIME_US) ? 1 : 0;
}

int boot_perf_is_sub_50ms(const boot_perf_metrics_t *metrics) {
    if (!metrics) return 0;
    return metrics->is_sub_50ms;
}
