/*
 * test_boot_perf.c -- Unit tests for boot timing and performance benchmark counters.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "../src/boot_perf.h"

static void test_boot_perf_init_clears_metrics(void) {
    boot_perf_metrics_t metrics;
    boot_perf_init(&metrics);
    assert(metrics.boot_cycles == 0);
    assert(metrics.is_sub_50ms == 0);
    printf("PASS: test_boot_perf_init_clears_metrics\n");
}

static void test_boot_perf_records_fast_boot(void) {
    boot_perf_metrics_t metrics;
    boot_perf_init(&metrics);

    /* Record 1,000,000 cycles (~2.85ms at 350MHz) */
    boot_perf_record_boot(&metrics, 1000000u);
    assert(metrics.boot_cycles == 1000000u);
    assert(metrics.is_sub_50ms == 1);
    assert(metrics.boot_time_us < 50000u);
    printf("PASS: test_boot_perf_records_fast_boot\n");
}

static void test_boot_perf_detects_slow_boot(void) {
    boot_perf_metrics_t metrics;
    boot_perf_init(&metrics);

    /* Record 20,000,000 cycles (~57.1ms at 350MHz) */
    boot_perf_record_boot(&metrics, 20000000u);
    assert(metrics.boot_cycles == 20000000u);
    assert(metrics.is_sub_50ms == 0);
    assert(metrics.boot_time_us > 50000u);
    printf("PASS: test_boot_perf_detects_slow_boot\n");
}

int main(void) {
    test_boot_perf_init_clears_metrics();
    test_boot_perf_records_fast_boot();
    test_boot_perf_detects_slow_boot();
    printf("All boot_perf tests passed cleanly!\n");
    return 0;
}
