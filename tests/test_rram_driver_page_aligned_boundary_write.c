/*
 * tests/test_rram_driver_page_aligned_boundary_write.c -- regression
 * test proving a write that ends exactly at the top of a page-aligned
 * backing-store region succeeds (real bounds-checking edge case).
 *
 * Related gap found this session: rram_write()'s ragged-end path calls
 * rram_write_page(), which re-checks addr_in_bounds(page_addr,
 * RRAM_PAGE_SIZE) -- requiring a FULL RRAM_PAGE_SIZE-byte page to fit
 * within the attached region, not just the caller's requested len. If
 * the backing-store's `size` (rram_driver_attach_backing_store()) is
 * NOT a multiple of RRAM_PAGE_SIZE, a legitimately in-bounds write
 * ending near the top of the region can spuriously fail with
 * RRAM_ERR_OUT_OF_BOUNDS, because the aligned page containing the
 * ragged-end bytes extends past the (non-page-aligned) region top.
 *
 * This is NEVER triggered by real usage: the only production caller
 * is rram_driver_attach_cartridge_partition(), which always uses
 * CARTRIDGE_TOTAL_SIZE (6 * 143360 = 860160 bytes), a clean multiple
 * of RRAM_PAGE_SIZE (32) with zero remainder -- confirmed by this
 * test. rram_driver.h's rram_driver_attach_backing_store() doc comment
 * now documents this as a size-alignment precondition rather than
 * leaving it an unstated assumption. This test is the regression lock
 * for the "page-aligned size -> boundary writes never spuriously fail"
 * guarantee that the real hardware caller actually relies on.
 */
#include <stdio.h>
#include <string.h>
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

int main(void) {
    /* Confirm the real production usage's size IS page-aligned -- the
     * precondition that makes the boundary-write case below always
     * succeed in practice. */
    CHECK((CARTRIDGE_TOTAL_SIZE % RRAM_PAGE_SIZE) == 0,
          "test_cartridge_total_size_is_page_aligned");

    /* A page-aligned-size region (mirrors the real cartridge partition's
     * invariant, just at a small synthetic scale). */
    #define REGION_SIZE (4 * 32u) /* 128 bytes, 4 pages, page-aligned */
    #define REGION_BASE 0x2000u
    static uint8_t store[REGION_SIZE];
    memset(store, 0, sizeof(store));
    rram_driver_attach_backing_store(store, REGION_BASE, REGION_SIZE);

    /* Write ending exactly at the last byte of the region -- addr
     * REGION_BASE + REGION_SIZE - 10 for 10 bytes, straddling into the
     * final page's ragged-end path. */
    uint8_t data[10] = {1,2,3,4,5,6,7,8,9,10};
    int rc = rram_write(REGION_BASE + REGION_SIZE - 10, data, sizeof(data));
    CHECK(rc == RRAM_OK,
          "test_write_ending_exactly_at_page_aligned_region_top_succeeds");

    uint8_t readback[10];
    rram_read(REGION_BASE + REGION_SIZE - 10, readback, sizeof(readback));
    CHECK(memcmp(readback, data, sizeof(data)) == 0,
          "test_write_ending_exactly_at_page_aligned_region_top_succeeds: readback matches");

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
