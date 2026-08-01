#include <stdio.h>
#include <string.h>

#include "../src/video_apple2.h"

/*
 * RED test (full-frame integration): decode all 192 rows of a Hi-Res frame
 * and verify EVERY row lands on its correct offset -- not just row 0/row 8
 * spot checks (tests/test_video_apple2.c) or single-byte color cases
 * (tests/test_video_apple2_color.c). Closes the coverage gap CLAUDE.md
 * warns about: "a subtly wrong line-offset table produces a
 * scrambled-looking but plausible image, easy to miss without a known-good
 * reference."
 *
 * Reference pattern: a full 8192-byte Hi-Res buffer where byte
 * buf[HIRES_BASE_ADDR + hires_line_offsets[row] + col] is deliberately set
 * to a value that encodes (row, col) so any misrouted row is immediately
 * detectable -- byte = (row & 0x7F) with bit 7 = (row is even). This is a
 * hand-constructed, independently-verifiable pattern (not derived by
 * reusing the code under test), matching CLAUDE.md's "write a test against
 * a known-good reference pattern... hand-verified byte pattern" guidance.
 */

#define HIRES_BUF_SIZE 8192

static uint8_t g_full_frame[HIRES_BUF_SIZE];

static uint8_t mock_read6502(uint16_t address) {
    uint16_t offset = address - HIRES_BASE_ADDR;
    if (offset < HIRES_BUF_SIZE) {
        return g_full_frame[offset];
    }
    return 0x00;
}

/* Independently computed row->offset (same formula as BRAINSTORM.md sec 2,
 * NOT calling into hires_line_offsets[] -- this must be a second, separate
 * derivation so the test can't just be checking the table against itself).
 */
static uint16_t expected_row_offset(int row) {
    return (uint16_t)(((row & 0x07) * 0x0400) +
                       (((row >> 3) & 0x07) * 0x0080) +
                       ((row >> 6) * 0x0028));
}

static int test_all_192_rows_decode_from_correct_offset(void) {
    memset(g_full_frame, 0x00, sizeof(g_full_frame));

    /* Encode byte 0 of each row with a per-row marker whose bit0 is ALWAYS
     * 1 (regardless of row) so that reading from any WRONG (unwritten,
     * defaults-to-0x00) offset reliably produces bit0=0 and fails loudly --
     * rather than risking a false pass when the wrong offset happens to
     * decode to the same expected bit by coincidence (e.g. an even row
     * landing on a still-zero byte). Bits 1-6 carry (row % 64) so a wrong
     * offset landing on a *different but already-written* row's byte is
     * also very likely to mismatch. */
    for (int row = 0; row < HIRES_ROWS; row++) {
        uint16_t off = expected_row_offset(row);
        uint8_t marker = (uint8_t)(0x01 | (((row % 64) & 0x3F) << 1));
        g_full_frame[off] = marker;
    }

    int failures = 0;
    for (int row = 0; row < HIRES_ROWS; row++) {
        uint8_t out_pixels[HIRES_PIXELS_WIDE];
        hires_decode_scanline_mono(row, mock_read6502, out_pixels);

        /* bit0 is always 1 in a correctly-written marker; reading from any
         * offset that wasn't written to (including a WRONG offset that
         * collides with a not-yet-written slot) yields byte 0x00 -> bit0 0.
         */
        if (out_pixels[0] != 1) {
            fprintf(stderr,
                    "FAIL: row %d pixel[0] = %u, expected 1 (offset table "
                    "likely wrong for this row -- got byte from wrong SRAM "
                    "address)\n",
                    row, out_pixels[0]);
            failures++;
            continue;
        }

        /* Also verify bits 1-6 (the row%64 payload) to catch the case where
         * the wrong offset happens to land on ANOTHER already-written row's
         * marker byte (both have bit0=1, but a different row%64 payload
         * would reveal the mismatch). */
        uint8_t expected_payload = (row % 64) & 0x3F;
        uint8_t got_payload = 0;
        for (int bit = 1; bit < 7; bit++) {
            got_payload |= (out_pixels[bit] << (bit - 1));
        }
        if (got_payload != expected_payload) {
            fprintf(stderr,
                    "FAIL: row %d payload = 0x%02X, expected 0x%02X "
                    "(offset table likely points at a different row's data)\n",
                    row, got_payload, expected_payload);
            failures++;
        }
    }

    if (failures == 0) {
        printf("PASS: test_all_192_rows_decode_from_correct_offset\n");
    }
    return failures;
}

static int test_hires_line_offsets_matches_independent_formula_for_all_rows(void) {
    int failures = 0;
    for (int row = 0; row < HIRES_ROWS; row++) {
        uint16_t expected = expected_row_offset(row);
        if (hires_line_offsets[row] != expected) {
            fprintf(stderr,
                    "FAIL: hires_line_offsets[%d] = 0x%04X, expected 0x%04X\n",
                    row, hires_line_offsets[row], expected);
            failures++;
        }
    }
    if (failures == 0) {
        printf("PASS: test_hires_line_offsets_matches_independent_formula_for_all_rows\n");
    }
    return failures;
}

int main(void) {
    int failures = 0;
    failures += test_hires_line_offsets_matches_independent_formula_for_all_rows();
    failures += test_all_192_rows_decode_from_correct_offset();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
