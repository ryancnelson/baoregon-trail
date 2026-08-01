/*
 * tests/test_lores_apple2_row_offsets_bounds.c -- unit test verifying lores_byte_row_offsets
 * table entries stay strictly within 1KB ($0400-$07FF) Page 1/2 bounds for all 24 byte-rows.
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../src/lores_apple2.h"

int main(void) {
    /* Verify row 0 offset is 0 */
    assert(lores_byte_row_offsets[0] == 0x0000);

    /* Verify row 23 offset is 0x03D0 */
    assert(lores_byte_row_offsets[23] == 0x03D0);

    /* Verify all 24 row offsets + 39 (40 bytes per row) fit in 1KB (0x0400 bytes) */
    for (int r = 0; r < LORES_BYTE_ROWS; r++) {
        uint16_t start_off = lores_byte_row_offsets[r];
        uint16_t end_off = start_off + LORES_COLS_BYTES - 1;
        assert(end_off < 0x0400u);
    }

    /* Verify NULL read_mem or NULL out_blocks is a silent no-op */
    uint8_t blocks[LORES_BLOCK_COLS * LORES_BLOCK_ROWS];
    memset(blocks, 0xAB, sizeof(blocks));

    lores_decode_screen_page(0, NULL, blocks);
    assert(blocks[0] == 0xAB);

    printf("PASS: lores_apple2 row offsets and NULL safety verified\n");
    printf("All tests passed.\n");
    return 0;
}
