/*
 * tests/test_boot_perf_cycles_to_us.c -- unit test for boot_perf_cycles_to_us API.
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "../src/boot_perf.h"

int main(void) {
    /* 0 cycles -> 0 us */
    assert(boot_perf_cycles_to_us(0) == 0);

    /* 350 cycles @ 350MHz -> 1 us */
    assert(boot_perf_cycles_to_us(350) == 1);

    /* 17,500,000 cycles @ 350MHz -> 50,000 us (50ms boundary) */
    assert(boot_perf_cycles_to_us(17500000ULL) == 50000);

    /* 35,000,000 cycles -> 100,000 us */
    assert(boot_perf_cycles_to_us(35000000ULL) == 100000);

    /* Cycle count with high 32 bits set returns UINT32_MAX safely */
    assert(boot_perf_cycles_to_us(0xFFFFFFFF00000000ULL) == 0xFFFFFFFFu);

    printf("PASS: boot_perf_cycles_to_us verified\n");
    printf("All tests passed.\n");
    return 0;
}
