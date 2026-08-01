/*
 * RED test: rram_driver.h's rram_read() docstring claims "out-of-bounds
 * reads are NOT range-checked here (matching the real driver...)" but
 * the actual implementation DOES call addr_in_bounds() and safely
 * no-ops on an out-of-range read -- this is a real doc/code mismatch
 * (verified independently of tests/test_rram_driver_read_bounds.c,
 * which already exercises this behavior but doesn't call out that the
 * header's own docstring contradicts it). Locks in the actual (safe)
 * behavior and the accompanying doc fix corrects the docstring to match.
 */
#include <stdio.h>
#include "../src/rram_driver.h"
#include "../src/cartridge_layout.h"

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

#define BACKING_STORE_SIZE (4 * 1024u)
#define BACKING_STORE_BASE  0x1000u
static uint8_t g_store[BACKING_STORE_SIZE];

static void test_docstring_matches_reality_out_of_bounds_read_is_safe_noop(void) {
    for (uint32_t i = 0; i < BACKING_STORE_SIZE; i++) {
        g_store[i] = 0;
    }
    rram_driver_attach_backing_store(g_store, BACKING_STORE_BASE, BACKING_STORE_SIZE);

    uint8_t out[16];
    for (int i = 0; i < 16; i++) {
        out[i] = 0xAA; /* sentinel: if the read actually happened (or
                          crashed reading garbage), this would change */
    }

    /* Read range starts below the attached region's base -- must be a
     * safe no-op (buffer left untouched), NOT an unchecked memcpy from
     * an out-of-bounds address as the (incorrect) docstring claimed. */
    rram_read(BACKING_STORE_BASE - 100, out, 16);

    int untouched = 1;
    for (int i = 0; i < 16; i++) {
        if (out[i] != 0xAA) {
            untouched = 0;
            break;
        }
    }
    CHECK(untouched, "test_docstring_matches_reality_out_of_bounds_read_is_safe_noop: "
                      "buffer untouched after out-of-bounds read attempt");
}

int main(void) {
    test_docstring_matches_reality_out_of_bounds_read_is_safe_noop();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
