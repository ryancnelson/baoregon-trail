/*
 * tests/test_cartridge_layout_bounds_safety.c -- unit test verifying
 * cartridge_layout_get_slot() bounds safety for valid (0..5) and out-of-bounds
 * indices (-1, 6, 99).
 */
#include <assert.h>
#include <stdio.h>

#include "../src/cartridge_layout.h"

int main(void) {
    /* Valid slots 0 through 5 */
    for (int i = 0; i < CARTRIDGE_SLOT_COUNT; i++) {
        const cartridge_slot_t *slot = cartridge_layout_get_slot(i);
        assert(slot != NULL);
        assert(slot->title != NULL);
        assert(slot->reram_addr == CARTRIDGE_RERAM_BASE + i * CARTRIDGE_SLOT_SIZE);
    }

    /* Out of bounds: negative index */
    assert(cartridge_layout_get_slot(-1) == NULL);
    assert(cartridge_layout_get_slot(-100) == NULL);

    /* Out of bounds: index >= CARTRIDGE_SLOT_COUNT */
    assert(cartridge_layout_get_slot(CARTRIDGE_SLOT_COUNT) == NULL);
    assert(cartridge_layout_get_slot(6) == NULL);
    assert(cartridge_layout_get_slot(99) == NULL);

    printf("PASS: cartridge_layout_get_slot bounds safety verified\n");
    printf("All tests passed.\n");
    return 0;
}
