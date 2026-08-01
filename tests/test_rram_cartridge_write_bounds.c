/*
 * tests/test_rram_cartridge_write_bounds.c -- unit test verifying
 * rram_write() bounds enforcement when attached to the cartridge partition
 * via rram_driver_attach_cartridge_partition().
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../src/rram_driver.h"
#include "../src/cartridge_layout.h"

int main(void) {
    uint8_t store[CARTRIDGE_TOTAL_SIZE];
    memset(store, 0x00, sizeof(store));

    rram_driver_attach_cartridge_partition(store);

    uint8_t payload[100];
    memset(payload, 0xAB, sizeof(payload));

    /* Valid write at partition start */
    assert(rram_write(CARTRIDGE_RERAM_BASE, payload, sizeof(payload)) == RRAM_OK);

    /* Valid write at partition end boundary */
    uint32_t end_addr = CARTRIDGE_RERAM_BASE + CARTRIDGE_TOTAL_SIZE - sizeof(payload);
    assert(rram_write(end_addr, payload, sizeof(payload)) == RRAM_OK);

    /* Invalid write overlapping past partition end boundary */
    uint32_t oob_end = CARTRIDGE_RERAM_BASE + CARTRIDGE_TOTAL_SIZE - 50;
    assert(rram_write(oob_end, payload, sizeof(payload)) == RRAM_ERR_OUT_OF_BOUNDS);

    /* Invalid write before partition start (headroom region) */
    assert(rram_write(CARTRIDGE_RERAM_ORIGIN, payload, sizeof(payload)) == RRAM_ERR_OUT_OF_BOUNDS);

    printf("PASS: rram_write cartridge partition bounds enforcement verified\n");
    printf("All tests passed.\n");
    return 0;
}
