/*
 * tests/test_boot_perf_safety.c -- Unit test for boot_perf NULL safety and large cycle overflow protection.
 */
#include <assert.h>
#include <stdio.h>
#include "boot_perf.h"

static void test_boot_perf_null_and_overflow_safety(void) {
    /* NULL pointer safety checks */
    boot_perf_init(NULL);
    boot_perf_record_boot(NULL, 1000);
    assert(boot_perf_is_sub_50ms(NULL) == 0);

    boot_perf_metrics_t m;
    boot_perf_init(&m);
    assert(boot_perf_is_sub_50ms(&m) == 0);

    /* Normal sub-50ms boot: 17,500,000 cycles at 350MHz = 50,000us = 50ms */
    /* 3,500,000 cycles at 350MHz = 10,000us = 10ms (< 50ms) */
    boot_perf_record_boot(&m, 3500000ULL);
    assert(m.boot_time_us == 10000);
    assert(m.is_sub_50ms == 1);
    assert(boot_perf_is_sub_50ms(&m) == 1);

    /* Large overflow cycle count (> 32-bit): > 50ms */
    boot_perf_record_boot(&m, 0xFFFFFFFF00000000ULL);
    assert(m.is_sub_50ms == 0);
    assert(boot_perf_is_sub_50ms(&m) == 0);

    printf("PASS: test_boot_perf_null_and_overflow_safety\n");
}

int main(void) {
    test_boot_perf_null_and_overflow_safety();
    return 0;
}
