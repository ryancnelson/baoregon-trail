/*
 * tests/test_boot_perf_null_metrics.c -- unit test verifying NULL pointer safety
 * and 50ms boundary behavior in boot_perf module.
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "../src/boot_perf.h"

int main(void) {
    /* NULL pointer safety */
    boot_perf_init(NULL);
    boot_perf_record_boot(NULL, 1000ULL);
    assert(boot_perf_is_sub_50ms(NULL) == 0);

    boot_perf_metrics_t m;
    boot_perf_init(&m);

    /* 0 cycles -> 0 us -> sub 50ms is TRUE */
    boot_perf_record_boot(&m, 0ULL);
    assert(m.boot_time_us == 0u);
    assert(boot_perf_is_sub_50ms(&m) == 1);

    /* Exact 50ms boundary = 17,500,000 cycles (350 cycles/us * 50,000 us) -> sub 50ms is FALSE (< 50ms required) */
    boot_perf_record_boot(&m, 17500000ULL);
    assert(m.boot_time_us == 50000u);
    assert(boot_perf_is_sub_50ms(&m) == 0);

    /* 49.997 ms = 17,499,000 cycles -> sub 50ms is TRUE */
    boot_perf_record_boot(&m, 17499000ULL);
    assert(m.boot_time_us == 49997u);
    assert(boot_perf_is_sub_50ms(&m) == 1);

    printf("PASS: boot_perf NULL metrics safety and 50ms boundary verified\n");
    printf("All tests passed.\n");
    return 0;
}
