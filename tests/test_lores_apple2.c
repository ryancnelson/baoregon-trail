#include <stdio.h>
#include <string.h>

#include "../src/lores_apple2.h"

/*
 * RED test (vertical tracer bullet): prove the Lo-Res byte-row offset
 * table and nibble-to-block-color expansion are correct for hand-verified
 * bytes, using a mock read6502 (same pattern as the Hi-Res tests).
 *
 * Byte format: low nibble (bits 0-3) = top block's color index, high
 * nibble (bits 4-7) = bottom block's color index. Byte 0x3F ->
 * top=0xF (white), bottom=0x3 (dark blue) per standard Apple II Lo-Res
 * palette ordering (index values only tested here, not RGB mapping).
 */

static uint8_t g_mock_page1[1024];
static uint8_t g_mock_page2[1024];

static uint8_t mock_read6502(uint16_t address) {
    if (address >= LORES_PAGE2_BASE_ADDR && address < LORES_PAGE2_BASE_ADDR + sizeof(g_mock_page2)) {
        return g_mock_page2[address - LORES_PAGE2_BASE_ADDR];
    }
    if (address >= LORES_PAGE1_BASE_ADDR && address < LORES_PAGE1_BASE_ADDR + sizeof(g_mock_page1)) {
        return g_mock_page1[address - LORES_PAGE1_BASE_ADDR];
    }
    return 0x00;
}

static int test_decode_screen_page1_single_byte_splits_into_two_blocks(void) {
    memset(g_mock_page1, 0x00, sizeof(g_mock_page1));
    memset(g_mock_page2, 0x00, sizeof(g_mock_page2));
    g_mock_page1[0] = 0x3F; /* byte-row 0, col 0: low nibble 0xF, high nibble 0x3 */

    uint8_t out_blocks[LORES_BLOCK_COLS * LORES_BLOCK_ROWS];
    memset(out_blocks, 0xFF, sizeof(out_blocks)); /* poison */

    lores_decode_screen_page(0, mock_read6502, out_blocks);

    /* Byte-row 0 covers visible block rows 0 (top nibble) and 1 (bottom
     * nibble); col 0 in both. */
    uint8_t top_block = out_blocks[0 * LORES_BLOCK_COLS + 0];
    uint8_t bottom_block = out_blocks[1 * LORES_BLOCK_COLS + 0];

    if (top_block != 0x0F) {
        fprintf(stderr, "FAIL: top block = 0x%X, expected 0xF\n", top_block);
        return 1;
    }
    if (bottom_block != 0x03) {
        fprintf(stderr, "FAIL: bottom block = 0x%X, expected 0x3\n", bottom_block);
        return 1;
    }
    printf("PASS: test_decode_screen_page1_single_byte_splits_into_two_blocks\n");
    return 0;
}

static int test_decode_screen_byte_row_uses_correct_interleaved_offset(void) {
    memset(g_mock_page1, 0x00, sizeof(g_mock_page1));
    memset(g_mock_page2, 0x00, sizeof(g_mock_page2));
    /* byte-row 8 lives at offset 0x28 per the interleave formula
     * (row%8=0, row/8=1 -> 0*0x80 + 1*0x28 = 0x28). Write a marker there. */
    g_mock_page1[0x28] = 0x21; /* top nibble 0x1, bottom nibble 0x2 */

    uint8_t out_blocks[LORES_BLOCK_COLS * LORES_BLOCK_ROWS];
    lores_decode_screen_page(0, mock_read6502, out_blocks);

    /* Byte-row 8 covers visible block rows 16 (8*2) and 17. */
    uint8_t top_block = out_blocks[16 * LORES_BLOCK_COLS + 0];
    uint8_t bottom_block = out_blocks[17 * LORES_BLOCK_COLS + 0];

    if (top_block != 0x01) {
        fprintf(stderr, "FAIL: byte-row 8 top block = 0x%X, expected 0x1\n", top_block);
        return 1;
    }
    if (bottom_block != 0x02) {
        fprintf(stderr, "FAIL: byte-row 8 bottom block = 0x%X, expected 0x2\n", bottom_block);
        return 1;
    }
    printf("PASS: test_decode_screen_byte_row_uses_correct_interleaved_offset\n");
    return 0;
}

static int test_decode_screen_page2_reads_from_0x0800_base(void) {
    memset(g_mock_page1, 0x00, sizeof(g_mock_page1));
    memset(g_mock_page2, 0x00, sizeof(g_mock_page2));
    g_mock_page1[0] = 0xAA; /* page 1: must NOT show up when page2=1 */
    g_mock_page2[0] = 0x3F; /* page 2: top=0xF, bottom=0x3 */

    uint8_t out_blocks[LORES_BLOCK_COLS * LORES_BLOCK_ROWS];
    lores_decode_screen_page(/*page2=*/1, mock_read6502, out_blocks);

    uint8_t top_block = out_blocks[0 * LORES_BLOCK_COLS + 0];
    uint8_t bottom_block = out_blocks[1 * LORES_BLOCK_COLS + 0];

    if (top_block != 0x0F || bottom_block != 0x03) {
        fprintf(stderr, "FAIL: page2 top=0x%X bottom=0x%X, expected 0xF/0x3\n",
                top_block, bottom_block);
        return 1;
    }
    printf("PASS: test_decode_screen_page2_reads_from_0x0800_base\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_decode_screen_page1_single_byte_splits_into_two_blocks();
    failures += test_decode_screen_byte_row_uses_correct_interleaved_offset();
    failures += test_decode_screen_page2_reads_from_0x0800_base();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
