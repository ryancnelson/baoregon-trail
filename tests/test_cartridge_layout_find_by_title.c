/*
 * tests/test_cartridge_layout_find_by_title.c -- unit test for cartridge_layout_find_slot_by_title.
 */
#include <assert.h>
#include <stdio.h>
#include <stddef.h>

#include "../src/cartridge_layout.h"

int main(void) {
    /* Test finding slot 0 by exact title */
    const cartridge_slot_t *slot0 = cartridge_layout_find_slot_by_title("The Oregon Trail (1985)");
    assert(slot0 != NULL);
    assert(slot0->reram_addr == CARTRIDGE_RERAM_BASE);

    /* Test finding slot 5 by exact title */
    const cartridge_slot_t *slot5 = cartridge_layout_find_slot_by_title("Ultima IV");
    assert(slot5 != NULL);
    assert(slot5->reram_addr == CARTRIDGE_RERAM_BASE + 5 * CARTRIDGE_SLOT_SIZE);

    /* Test nonexistent title returns NULL */
    const cartridge_slot_t *bad = cartridge_layout_find_slot_by_title("Nonexistent Game");
    assert(bad == NULL);

    /* Test NULL title returns NULL safely */
    const cartridge_slot_t *null_title = cartridge_layout_find_slot_by_title(NULL);
    assert(null_title == NULL);

    printf("PASS: cartridge_layout_find_slot_by_title verified\n");
    printf("All tests passed.\n");
    return 0;
}
