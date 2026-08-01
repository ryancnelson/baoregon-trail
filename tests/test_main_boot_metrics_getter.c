/*
 * tests/test_main_boot_metrics_getter.c -- Unit test for baoregon_get_boot_metrics.
 */
#include <assert.h>
#include <stdio.h>
#include "boot_perf.h"

void baoregon_main_init(uint64_t boot_cycles);
const boot_perf_metrics_t *baoregon_get_boot_metrics(void);

static void test_get_boot_metrics(void) {
    /* Initialize with 3,500,000 cycles = 10,000us = 10ms */
    baoregon_main_init(3500000ULL);

    const boot_perf_metrics_t *m = baoregon_get_boot_metrics();
    assert(m != NULL);
    assert(m->boot_cycles == 3500000ULL);
    assert(m->boot_time_us == 10000);
    assert(m->is_sub_50ms == 1);

    printf("PASS: test_get_boot_metrics\n");
}

int main(void) {
    test_get_boot_metrics();
    return 0;
}
