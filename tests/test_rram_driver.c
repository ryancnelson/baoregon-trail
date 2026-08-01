/*
 * RED test: rram_driver.c, a host-testable port of armstrongsubero/
 * dabao-sdk's hardware-verified ReRAM driver algorithm (real hardware
 * source: src/bao1x/hardware_rram/rram.c, referenced via Discord #c-side
 * findings 2026-07-31). Backed by a plain in-memory buffer instead of real
 * RRC MMIO -- proves bounds-checking, page alignment, ragged-start/end
 * read-modify-write, and readback verification are correct before wiring
 * to real hardware registers.
 */
#include <stdio.h>
#include <string.h>
#include "../src/rram_driver.h"

static int failures = 0;

#define CHECK(cond, label) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", label); \
            failures++; \
        } else { \
            printf("PASS: %s\n", label); \
        } \
    } while (0)

#define BACKING_STORE_SIZE (4 * 1024u) /* small synthetic region, not the real 4MB */
#define BACKING_STORE_BASE  0x1000u

static uint8_t g_store[BACKING_STORE_SIZE];

static void reset_store(void) {
    memset(g_store, 0, sizeof(g_store));
    rram_driver_attach_backing_store(g_store, BACKING_STORE_BASE, BACKING_STORE_SIZE);
}

static void test_read_write_page_roundtrip_aligned(void) {
    reset_store();
    uint8_t page[RRAM_PAGE_SIZE];
    for (int i = 0; i < (int)RRAM_PAGE_SIZE; i++) {
        page[i] = (uint8_t)(i + 1);
    }

    int rc = rram_write_page(BACKING_STORE_BASE, page);
    CHECK(rc == RRAM_OK, "test_read_write_page_roundtrip_aligned: write succeeds");

    uint8_t readback[RRAM_PAGE_SIZE];
    rram_read(BACKING_STORE_BASE, readback, RRAM_PAGE_SIZE);
    CHECK(memcmp(readback, page, RRAM_PAGE_SIZE) == 0,
          "test_read_write_page_roundtrip_aligned: readback matches");
}

static void test_write_page_rejects_unaligned_address(void) {
    reset_store();
    uint8_t page[RRAM_PAGE_SIZE] = {0};

    int rc = rram_write_page(BACKING_STORE_BASE + 1, page); /* not page-aligned */
    CHECK(rc == RRAM_ERR_OUT_OF_BOUNDS,
          "test_write_page_rejects_unaligned_address");
}

static void test_write_page_rejects_out_of_bounds(void) {
    reset_store();
    uint8_t page[RRAM_PAGE_SIZE] = {0};

    /* Below the attached region's base. */
    int rc_low = rram_write_page(BACKING_STORE_BASE - RRAM_PAGE_SIZE, page);
    CHECK(rc_low == RRAM_ERR_OUT_OF_BOUNDS,
          "test_write_page_rejects_out_of_bounds: below base rejected");

    /* At/beyond the top of the attached region. */
    uint32_t top_page = BACKING_STORE_BASE + BACKING_STORE_SIZE;
    int rc_high = rram_write_page(top_page, page);
    CHECK(rc_high == RRAM_ERR_OUT_OF_BOUNDS,
          "test_write_page_rejects_out_of_bounds: at/beyond top rejected");
}

static void test_write_unaligned_start_uses_read_modify_write(void) {
    /* Port of dabao-sdk's rram_example.c Test 3: write 10 bytes at an
     * unaligned offset (+5) within a page and confirm the surrounding
     * bytes of that page are preserved (read-modify-write), not clobbered. */
    reset_store();

    uint8_t initial_page[RRAM_PAGE_SIZE];
    for (int i = 0; i < (int)RRAM_PAGE_SIZE; i++) {
        initial_page[i] = 0xAA;
    }
    CHECK(rram_write_page(BACKING_STORE_BASE, initial_page) == RRAM_OK,
          "test_write_unaligned_start_uses_read_modify_write: seed page write");

    uint8_t small[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE, 0x12, 0x34};
    int rc = rram_write(BACKING_STORE_BASE + 5, small, sizeof(small));
    CHECK(rc == RRAM_OK, "test_write_unaligned_start_uses_read_modify_write: unaligned write succeeds");

    uint8_t readback[RRAM_PAGE_SIZE];
    rram_read(BACKING_STORE_BASE, readback, RRAM_PAGE_SIZE);

    int ok = 1;
    /* Bytes 0-4 must be untouched (still 0xAA). */
    for (int i = 0; i < 5; i++) {
        if (readback[i] != 0xAA) {
            fprintf(stderr, "FAIL: byte %d before patch = 0x%02X, expected 0xAA (untouched)\n",
                    i, readback[i]);
            ok = 0;
        }
    }
    /* Bytes 5-14 must be the patched data. */
    for (int i = 0; i < 10; i++) {
        if (readback[5 + i] != small[i]) {
            fprintf(stderr, "FAIL: byte %d = 0x%02X, expected 0x%02X (patched)\n",
                    5 + i, readback[5 + i], small[i]);
            ok = 0;
        }
    }
    /* Bytes 15-31 must still be untouched (still 0xAA). */
    for (int i = 15; i < (int)RRAM_PAGE_SIZE; i++) {
        if (readback[i] != 0xAA) {
            fprintf(stderr, "FAIL: byte %d after patch = 0x%02X, expected 0xAA (untouched)\n",
                    i, readback[i]);
            ok = 0;
        }
    }
    CHECK(ok, "test_write_unaligned_start_uses_read_modify_write: surrounding bytes preserved");
}

static void test_write_spanning_multiple_pages(void) {
    /* Port of dabao-sdk's rram_example.c Test 4: write 48 bytes starting
     * at a page boundary -- spans one full page plus a ragged remainder
     * in the next page. */
    reset_store();

    uint8_t big[48];
    for (int i = 0; i < 48; i++) {
        big[i] = (uint8_t)(0xA0 + i);
    }

    int rc = rram_write(BACKING_STORE_BASE, big, sizeof(big));
    CHECK(rc == RRAM_OK, "test_write_spanning_multiple_pages: write succeeds");

    uint8_t readback[48];
    rram_read(BACKING_STORE_BASE, readback, sizeof(readback));
    CHECK(memcmp(readback, big, sizeof(big)) == 0,
          "test_write_spanning_multiple_pages: readback matches across page boundary");
}

static void test_write_rejects_out_of_bounds_range(void) {
    reset_store();
    uint8_t data[16] = {0};

    /* Range that starts in-bounds but extends past the top of the region. */
    int rc = rram_write(BACKING_STORE_BASE + BACKING_STORE_SIZE - 8, data, sizeof(data));
    CHECK(rc == RRAM_ERR_OUT_OF_BOUNDS,
          "test_write_rejects_out_of_bounds_range");
}

static void test_erase_page_writes_all_0xff(void) {
    reset_store();
    uint8_t page[RRAM_PAGE_SIZE];
    for (int i = 0; i < (int)RRAM_PAGE_SIZE; i++) {
        page[i] = (uint8_t)i; /* non-0xFF starting content */
    }
    rram_write_page(BACKING_STORE_BASE, page);

    int rc = rram_erase_page(BACKING_STORE_BASE);
    CHECK(rc == RRAM_OK, "test_erase_page_writes_all_0xff: erase succeeds");

    uint8_t readback[RRAM_PAGE_SIZE];
    rram_read(BACKING_STORE_BASE, readback, RRAM_PAGE_SIZE);

    int all_ff = 1;
    for (int i = 0; i < (int)RRAM_PAGE_SIZE; i++) {
        if (readback[i] != 0xFF) {
            all_ff = 0;
            break;
        }
    }
    CHECK(all_ff, "test_erase_page_writes_all_0xff: readback is all 0xFF");
}

static void test_write_zero_length_is_a_noop(void) {
    reset_store();
    int rc = rram_write(BACKING_STORE_BASE, NULL, 0);
    CHECK(rc == RRAM_OK, "test_write_zero_length_is_a_noop");
}

int main(void) {
    test_read_write_page_roundtrip_aligned();
    test_write_page_rejects_unaligned_address();
    test_write_page_rejects_out_of_bounds();
    test_write_unaligned_start_uses_read_modify_write();
    test_write_spanning_multiple_pages();
    test_write_rejects_out_of_bounds_range();
    test_erase_page_writes_all_0xff();
    test_write_zero_length_is_a_noop();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
