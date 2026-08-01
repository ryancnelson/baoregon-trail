/*
 * tests/test_rram_driver_read_bounds.c -- TDD test for rram_read out-of-bounds protection.
 */
#include <assert.h>
#include <stdio.h>
#include "rram_driver.h"
#include "cartridge_layout.h"

static uint8_t g_fake_reram[CARTRIDGE_TOTAL_SIZE];

static void test_rram_read_out_of_bounds_safely_ignored(void) {
    rram_driver_attach_cartridge_partition(g_fake_reram);

    uint8_t out[16] = {0xAA, 0xAA, 0xAA, 0xAA};

    /* Attempt out-of-bounds read below base address */
    rram_read(CARTRIDGE_RERAM_BASE - 100, out, 16);
    assert(out[0] == 0xAA); /* Buffer must remain untouched */

    /* Attempt out-of-bounds read past end of region */
    rram_read(CARTRIDGE_RERAM_BASE + CARTRIDGE_TOTAL_SIZE + 10, out, 16);
    assert(out[0] == 0xAA); /* Buffer must remain untouched */

    printf("PASS: test_rram_read_out_of_bounds_safely_ignored\n");
}

int main(void) {
    test_rram_read_out_of_bounds_safely_ignored();
    return 0;
}
