#include <stdio.h>
#include <string.h>

#include "../src/lores_apple2.h"

/*
 * RED test: lores_decode_screen_page() had no NULL checks on
 * read_mem/out_blocks -- same gap class as video_apple2.c, hardened to
 * match the team's "safe no-op on bad input" convention.
 */

static uint8_t mock_read6502(uint16_t address) {
    (void)address;
    return 0xFF;
}

static int test_null_read_mem_is_safe_noop(void) {
    uint8_t out_blocks[LORES_BLOCK_COLS * LORES_BLOCK_ROWS];
    memset(out_blocks, 0xAA, sizeof(out_blocks));

    lores_decode_screen_page(0, NULL, out_blocks); /* must not segfault */

    for (int i = 0; i < LORES_BLOCK_COLS * LORES_BLOCK_ROWS; i++) {
        if (out_blocks[i] != 0xAA) {
            fprintf(stderr, "FAIL: out_blocks[%d] = %u, expected untouched poison 0xAA\n",
                    i, out_blocks[i]);
            return 1;
        }
    }
    printf("PASS: test_null_read_mem_is_safe_noop\n");
    return 0;
}

static int test_null_out_blocks_is_safe_noop(void) {
    lores_decode_screen_page(0, mock_read6502, NULL); /* must not segfault */
    printf("PASS: test_null_out_blocks_is_safe_noop\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_null_read_mem_is_safe_noop();
    failures += test_null_out_blocks_is_safe_noop();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
