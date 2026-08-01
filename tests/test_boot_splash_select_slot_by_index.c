/*
 * tests/test_boot_splash_select_slot_by_index.c -- unit test for boot_splash_select_slot_by_index.
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#include "../src/boot_splash.h"

static const uint8_t *g_captured_image = NULL;

static void mock_disk_image_setter(const uint8_t *image) {
    g_captured_image = image;
}

int main(void) {
    boot_splash_state_t state;
    boot_splash_init(&state);

    /* 1. Select valid index (index 3) */
    g_captured_image = NULL;
    int res = boot_splash_select_slot_by_index(&state, 3, mock_disk_image_setter);
    assert(res == 1);
    assert(state.selected_index == 3);
    assert(g_captured_image == (const uint8_t *)(uintptr_t)(CARTRIDGE_RERAM_BASE + 3 * CARTRIDGE_SLOT_SIZE));

    /* 2. Select negative index (clamped to 0) */
    g_captured_image = NULL;
    res = boot_splash_select_slot_by_index(&state, -5, mock_disk_image_setter);
    assert(res == 1);
    assert(state.selected_index == 0);
    assert(g_captured_image == (const uint8_t *)(uintptr_t)CARTRIDGE_RERAM_BASE);

    /* 3. Select out-of-bounds high index (clamped to CARTRIDGE_SLOT_COUNT - 1 = 5) */
    g_captured_image = NULL;
    res = boot_splash_select_slot_by_index(&state, 99, mock_disk_image_setter);
    assert(res == 1);
    assert(state.selected_index == 5);
    assert(g_captured_image == (const uint8_t *)(uintptr_t)(CARTRIDGE_RERAM_BASE + 5 * CARTRIDGE_SLOT_SIZE));

    /* 4. NULL on_select returns 0 */
    res = boot_splash_select_slot_by_index(&state, 2, NULL);
    assert(res == 0);

    /* 5. NULL state returns 0 */
    res = boot_splash_select_slot_by_index(NULL, 2, mock_disk_image_setter);
    assert(res == 0);

    printf("PASS: boot_splash_select_slot_by_index verified\n");
    printf("All tests passed.\n");
    return 0;
}
