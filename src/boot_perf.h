/*
 * boot_perf.h -- Boot timing and performance benchmark interface for Baochip-1x.
 */
#ifndef BOOT_PERF_H
#define BOOT_PERF_H

#include <stdint.h>

#define BAOCHIP_MAIN_CPU_FREQ_HZ 350000000u
#define BAOCHIP_MAX_BOOT_TIME_US 50000u
#define BAOCHIP_MAX_BOOT_CYCLES  ((uint64_t)BAOCHIP_MAIN_CPU_FREQ_HZ * BAOCHIP_MAX_BOOT_TIME_US / 1000000u)

typedef struct {
    uint64_t boot_cycles;
    uint32_t boot_time_us;
    int is_sub_50ms;
} boot_perf_metrics_t;

void boot_perf_init(boot_perf_metrics_t *metrics);
void boot_perf_record_boot(boot_perf_metrics_t *metrics, uint64_t cycles);

#endif /* BOOT_PERF_H */
