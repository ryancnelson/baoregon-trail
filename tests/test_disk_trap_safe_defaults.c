/*
 * RED test: disk_trap.h documents disk_trap_read_byte()'s behavior as
 * "undefined" if disk_trap_select_sector() has not been called
 * successfully first, or if byte_offset >= DOS33_SECTOR_SIZE. Neither
 * claim matches the actual implementation:
 *
 *   1. byte_offset is a uint8_t (0-255); DOS33_SECTOR_SIZE is 256, so
 *      byte_offset >= DOS33_SECTOR_SIZE is mathematically IMPOSSIBLE --
 *      that clause describes a case that can never occur, not an actual
 *      undefined-behavior risk.
 *   2. disk_trap_read_byte() actually checks g_have_selection and
 *      g_disk_image internally and safely returns 0x00 if no valid
 *      selection/image is set -- this is DEFINED, safe behavior, not
 *      undefined.
 *
 * This test locks in the real (safe) behavior as a regression, and the
 * accompanying doc fix corrects disk_trap.h to describe what the code
 * actually does instead of overstating undefined-behavior risk that
 * doesn't exist -- a caller reading the old docs might add unnecessary
 * defensive guards, or worse, distrust otherwise-safe code.
 */
#include <stdio.h>
#include "../src/disk_trap.h"

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

static void test_read_byte_before_any_selection_returns_zero_safely(void) {
    /* No disk_trap_set_image() or disk_trap_select_sector() call has
     * happened yet in this test binary's process -- this is the very
     * first disk_trap call. Must return 0x00, not crash or read garbage. */
    uint8_t result = disk_trap_read_byte(0);
    CHECK(result == 0x00,
          "test_read_byte_before_any_selection_returns_zero_safely: "
          "reading before any selection returns 0x00, not undefined behavior");
}

static void test_read_byte_with_image_set_but_no_sector_selected_returns_zero(void) {
    static uint8_t dummy_image[DOS33_DISK_IMAGE_SIZE];
    for (uint32_t i = 0; i < DOS33_DISK_IMAGE_SIZE; i++) {
        dummy_image[i] = 0xAB; /* distinct from 0x00 so a leak would show up */
    }
    disk_trap_set_image(dummy_image);

    /* Image is set, but no sector has been selected via
     * disk_trap_select_sector() in this fresh scenario -- must still
     * safely return 0x00, not leak dummy_image's 0xAB content. */
    uint8_t result = disk_trap_read_byte(5);
    CHECK(result == 0x00,
          "test_read_byte_with_image_set_but_no_sector_selected_returns_zero");
}

static void test_read_byte_after_failed_select_still_returns_zero(void) {
    static uint8_t dummy_image[DOS33_DISK_IMAGE_SIZE];
    for (uint32_t i = 0; i < DOS33_DISK_IMAGE_SIZE; i++) {
        dummy_image[i] = 0xCD;
    }
    disk_trap_set_image(dummy_image);

    /* Attempt an invalid select (out-of-range track) -- must fail and
     * leave no valid selection behind, so a read still safely returns
     * 0x00 rather than reading garbage/uninitialized offset math. */
    int rc = disk_trap_select_sector(35, 0); /* DOS33_TRACKS == 35, so 35 is out of range */
    CHECK(rc != 0, "test_read_byte_after_failed_select_still_returns_zero: invalid select rejected");

    uint8_t result = disk_trap_read_byte(0);
    CHECK(result == 0x00,
          "test_read_byte_after_failed_select_still_returns_zero: read after failed select is safe");
}

static void test_byte_offset_range_0_to_255_is_the_full_uint8_t_range(void) {
    /* Documents/locks in that byte_offset's uint8_t type makes
     * "byte_offset >= DOS33_SECTOR_SIZE" (256) mathematically
     * unreachable -- the type itself is the guarantee, not a runtime
     * check anywhere in disk_trap.c. */
    CHECK(DOS33_SECTOR_SIZE == 256,
          "test_byte_offset_range_0_to_255_is_the_full_uint8_t_range: sector size is 256");
    /* uint8_t's max value (255) is always < 256 -- this is a compile-time
     * truth, asserted here so the reasoning is visible in test output. */
    uint8_t max_byte_offset = 255;
    CHECK((uint32_t)max_byte_offset < (uint32_t)DOS33_SECTOR_SIZE,
          "test_byte_offset_range_0_to_255_is_the_full_uint8_t_range: "
          "uint8_t max (255) is always < DOS33_SECTOR_SIZE (256), so the "
          "invalid-byte_offset case documented in disk_trap.h can never occur");
}

int main(void) {
    test_read_byte_before_any_selection_returns_zero_safely();
    test_read_byte_with_image_set_but_no_sector_selected_returns_zero();
    test_read_byte_after_failed_select_still_returns_zero();
    test_byte_offset_range_0_to_255_is_the_full_uint8_t_range();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
