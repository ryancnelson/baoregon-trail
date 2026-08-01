/*
 * tests/test_rram_driver_erase_bounds_safety.c -- unit test verifying
 * rram_erase_page() returns RRAM_OK and populates 0xFF for aligned page,
 * and safely returns RRAM_ERR_OUT_OF_BOUNDS for unaligned or out-of-bounds addresses.
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

    /* Erase valid page-aligned page at partition base */
    int rc = rram_erase_page(CARTRIDGE_RERAM_BASE);
    assert(rc == RRAM_OK);

    /* Verify page bytes set to 0xFF */
    uint8_t readback[RRAM_PAGE_SIZE];
    rram_read(CARTRIDGE_RERAM_BASE, readback, RRAM_PAGE_SIZE);
    for (uint32_t i = 0; i < RRAM_PAGE_SIZE; i++) {
        assert(readback[i] == 0xFF);
    }

    /* Reject unaligned address (CARTRIDGE_RERAM_BASE + 7) */
    assert(rram_erase_page(CARTRIDGE_RERAM_BASE + 7) == RRAM_ERR_OUT_OF_BOUNDS);

    /* Reject out of bounds low address */
    assert(rram_erase_page(0x10000000) == RRAM_ERR_OUT_OF_BOUNDS);

    /* Reject out of bounds high address */
    assert(rram_erase_page(CARTRIDGE_RERAM_BASE + CARTRIDGE_TOTAL_SIZE) == RRAM_ERR_OUT_OF_BOUNDS);

    printf("PASS: rram_erase_page alignment and bounds safety verified\n");
    printf("All tests passed.\n");
    return 0;
}
