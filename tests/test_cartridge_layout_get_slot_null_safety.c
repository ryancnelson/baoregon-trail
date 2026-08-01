/*
 * tests/test_cartridge_layout_get_slot_null_safety.c -- unit test verifying
 * cartridge_layout_get_slot() returns valid pointers for 0..5 and NULL for out-of-bounds indices.
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../src/cartridge_layout.h"

int main(void) {
    /* Valid indices 0..5 */
    const cartridge_slot_t *slot0 = cartridge_layout_get_slot(0);
    assert(slot0 != NULL);
    assert(strcmp(slot0->title, "The Oregon Trail (1985)") == 0);
    assert(slot0->reram_addr == CARTRIDGE_RERAM_BASE);

    const cartridge_slot_t *slot5 = cartridge_layout_get_slot(5);
    assert(slot5 != NULL);
    assert(strcmp(slot5->title, "Ultima IV") == 0);
    assert(slot5->reram_addr == CARTRIDGE_RERAM_BASE + 5 * CARTRIDGE_SLOT_SIZE);

    /* Out of bounds negative indices */
    assert(cartridge_layout_get_slot(-1) == NULL);
    assert(cartridge_layout_get_slot(-100) == NULL);

    /* Out of bounds upper indices */
    assert(cartridge_layout_get_slot(6) == NULL);
    assert(cartridge_layout_get_slot(100) == NULL);

    printf("PASS: cartridge_layout_get_slot valid indexing and NULL safety verified\n");
    printf("All tests passed.\n");
    return 0;
}
