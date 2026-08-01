/*
 * RED test: disk_trap_set_image(NULL) safety.
 *
 * Following the crew's ongoing null-pointer-safety sweep pattern
 * (disk_sector_layout, boot_splash, boot_perf, bio_display, bunnie_audio,
 * video_apple2/lores_apple2 all recently hardened) -- verify
 * disk_trap_set_image(0) doesn't itself crash (it's just a pointer
 * store, so it shouldn't), and more importantly that calling
 * disk_trap_read_byte() AFTER explicitly clearing the image back to
 * NULL (e.g. a caller "unloading" a cartridge slot) safely returns
 * 0x00 rather than dereferencing the now-null g_disk_image, matching
 * disk_trap.h's documented safe-default contract
 * (tests/test_disk_trap_safe_defaults.c) for the "no image set" case.
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

static void test_set_image_null_does_not_crash(void) {
    /* Must not crash -- disk_trap_set_image() just stores the pointer,
     * it doesn't dereference it, so passing NULL should be a safe,
     * defined operation (effectively "no image loaded"). */
    disk_trap_set_image(0);
    printf("PASS: test_set_image_null_does_not_crash: no crash\n");
}

static void test_read_after_unloading_image_to_null_returns_zero_safely(void) {
    /* Load a real image, select a valid sector, confirm reads work --
     * then "unload" by setting the image back to NULL (simulating a
     * cartridge slot being cleared/game exiting) and confirm reads
     * become safely inert (0x00) again, not a stale-pointer crash or
     * leftover data from the previous image. */
    static uint8_t dummy_image[DOS33_DISK_IMAGE_SIZE];
    for (uint32_t i = 0; i < DOS33_DISK_IMAGE_SIZE; i++) {
        dummy_image[i] = 0xEE;
    }
    disk_trap_set_image(dummy_image);
    int rc = disk_trap_select_sector(0, 0);
    CHECK(rc == 0, "test_read_after_unloading_image_to_null_returns_zero_safely: initial select succeeds");
    CHECK(disk_trap_read_byte(0) == 0xEE,
          "test_read_after_unloading_image_to_null_returns_zero_safely: reads real image data before unload");

    disk_trap_set_image(0); /* unload */

    uint8_t result = disk_trap_read_byte(0);
    CHECK(result == 0x00,
          "test_read_after_unloading_image_to_null_returns_zero_safely: reads 0x00 after unload, not stale 0xEE");
}

int main(void) {
    test_set_image_null_does_not_crash();
    test_read_after_unloading_image_to_null_returns_zero_safely();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
