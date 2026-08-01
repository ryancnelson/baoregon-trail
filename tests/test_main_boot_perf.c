/*
 * tests/test_main_boot_perf.c -- Unit test for main boot timing benchmark integration.
 */
#include <assert.h>
#include <stdio.h>
#include "boot_perf.h"

extern boot_perf_metrics_t g_boot_metrics;
void baoregon_main_init(uint64_t boot_cycles);

static void test_main_init_records_sub_50ms_boot_performance(void) {
    baoregon_main_init(17500000ULL); /* 50ms at 350 MHz */
    assert(g_boot_metrics.boot_cycles == 17500000ULL);
    assert(g_boot_metrics.boot_time_us == 50000U);
    assert(g_boot_metrics.is_sub_50ms == 0); /* 50000 is not < 50000 */

    baoregon_main_init(10000000ULL); /* ~28.5ms at 350 MHz */
    assert(g_boot_metrics.boot_cycles == 10000000ULL);
    assert(g_boot_metrics.boot_time_us == 28571U);
    assert(g_boot_metrics.is_sub_50ms == 1);

    printf("PASS: test_main_init_records_sub_50ms_boot_performance\n");
}

int main(void) {
    test_main_init_records_sub_50ms_boot_performance();
    return 0;
}
