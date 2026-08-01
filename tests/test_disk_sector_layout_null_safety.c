/*
 * RED test: dos33_sector_offset() null-pointer safety for out_offset.
 *
 * Following the crew's recent null-pointer-safety sweep pattern
 * (video_apple2/lores_apple2 null pointer safety, bunnie_audio null
 * pointer safety) -- dos33_sector_offset() writes to *out_offset with no
 * NULL check on out_offset itself. A caller passing NULL (e.g. a
 * validate-only call that doesn't need the offset) would crash with a
 * null pointer write rather than getting a defined error return. This
 * closes that gap in Duke's own disk-layout module to match the safety
 * bar the rest of the codebase is being held to.
 */
#include <stdio.h>
#include "../src/disk_sector_layout.h"

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

static void test_null_out_offset_with_valid_track_sector_does_not_crash(void) {
    /* Valid (track, sector) but out_offset == NULL -- must not
     * dereference NULL. Documented contract: treat NULL out_offset as
     * an invalid-arguments case and return -1 (no offset to report to a
     * null destination), consistent with dos33_sector_offset()'s
     * existing "-1 on any invalid input" contract. */
    int rc = dos33_sector_offset(0, 0, 0);
    CHECK(rc == -1, "test_null_out_offset_with_valid_track_sector_does_not_crash: returns -1, doesn't crash");
}

static void test_null_out_offset_with_invalid_track_sector_still_does_not_crash(void) {
    /* Both problems at once: out-of-range track AND NULL out_offset --
     * must still return an error cleanly, not crash trying to report
     * through the null pointer path or the out-of-range path. */
    int rc = dos33_sector_offset(35, 0, 0); /* track 35 is out of range (max 34) */
    CHECK(rc == -1, "test_null_out_offset_with_invalid_track_sector_still_does_not_crash");
}

static void test_valid_call_with_real_pointer_still_works(void) {
    /* Regression guard: the NULL-check fix must not break the normal,
     * valid-pointer case. */
    uint32_t offset = 0xFFFFFFFFu;
    int rc = dos33_sector_offset(17, 0, &offset);
    CHECK(rc == 0, "test_valid_call_with_real_pointer_still_works: succeeds");
    CHECK(offset == 69632u, "test_valid_call_with_real_pointer_still_works: correct offset (VTOC)");
}

int main(void) {
    test_null_out_offset_with_valid_track_sector_does_not_crash();
    test_null_out_offset_with_invalid_track_sector_still_does_not_crash();
    test_valid_call_with_real_pointer_still_works();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
