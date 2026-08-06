/*
 * RED test: cartridge_layout.c's ReRAM game-slot table.
 *
 * Verifies (per CLAUDE.md's TDD discipline -- prove the real math before
 * trusting it, don't eyeball a doc's hex literal):
 *   1. The corrected 2.5 MiB partition base address is actually 2.5 MiB
 *      past ReRAM's origin (catches the exact math bug found in
 *      BRAINSTORM.md's original "2.5 MB (0x20080000)" example).
 *   2. Every slot's address matches base + index*slot_size (no gaps,
 *      no overlap, matches the documented order).
 *   3. The whole table fits within the physical 4 MiB ReRAM region
 *      (doesn't run off the end -- would silently corrupt whatever comes
 *      after ReRAM, or just not exist on real hardware).
 *   4. CARTRIDGE_SLOT_SIZE matches disk_sector_layout.h's
 *      DOS33_DISK_IMAGE_SIZE exactly (the two headers must stay in sync;
 *      this test is the cross-check since cartridge_layout.h deliberately
 *      doesn't #include disk_sector_layout.h).
 */
#include <stdio.h>
#include <string.h>
#include "../src/cartridge_layout.h"
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

static void test_partition_base_is_exactly_2_5_mib_past_reram_origin(void) {
    /* This is the specific bug found in BRAINSTORM.md: "2.5 MB
     * (0x20080000)" -- 0x80000 is 512 KiB, not 2.5 MiB. The corrected
     * value must be origin + 2.5*1024*1024 bytes exactly. */
    uint32_t expected_2_5_mib_in_bytes = (uint32_t)(2.5 * 1024 * 1024);
    CHECK(expected_2_5_mib_in_bytes == 0x00280000u,
          "test_partition_base_is_exactly_2_5_mib_past_reram_origin: 2.5 MiB == 0x280000");

    CHECK(CARTRIDGE_RERAM_BASE == CARTRIDGE_RERAM_ORIGIN + expected_2_5_mib_in_bytes,
          "test_partition_base_is_exactly_2_5_mib_past_reram_origin: base == origin + 2.5MiB");

    /* Explicitly confirm this is NOT the buggy address from
     * BRAINSTORM.md's original (uncorrected) example (adjusted for the
     * 2026-08-06 ReRAM base-address correction, 0x20000000 -> 0x60000000
     * -- see docs/baochip-1x-memory-map-findings.md). */
    CHECK(CARTRIDGE_RERAM_BASE != 0x60080000u,
          "test_partition_base_is_exactly_2_5_mib_past_reram_origin: is not the old buggy +0x80000 value");
    CHECK(CARTRIDGE_RERAM_BASE == 0x60280000u,
          "test_partition_base_is_exactly_2_5_mib_past_reram_origin: matches corrected 0x60280000");
}

static void test_slot_size_matches_dos33_disk_image_size(void) {
    CHECK(CARTRIDGE_SLOT_SIZE == (uint32_t)DOS33_DISK_IMAGE_SIZE,
          "test_slot_size_matches_dos33_disk_image_size");
}

static void test_slot_addresses_are_contiguous_with_no_gaps_or_overlap(void) {
    int ok = 1;
    for (int i = 0; i < CARTRIDGE_SLOT_COUNT; i++) {
        uint32_t expected = CARTRIDGE_RERAM_BASE + (uint32_t)i * CARTRIDGE_SLOT_SIZE;
        if (cartridge_slots[i].reram_addr != expected) {
            fprintf(stderr,
                    "FAIL: cartridge_slots[%d].reram_addr = 0x%08X, expected 0x%08X\n",
                    i, cartridge_slots[i].reram_addr, expected);
            ok = 0;
        }
    }
    CHECK(ok, "test_slot_addresses_are_contiguous_with_no_gaps_or_overlap");
}

static void test_slot_titles_match_brainstorm_order(void) {
    /* Per BRAINSTORM.md section 5's table, in order. */
    static const char *expected_titles[CARTRIDGE_SLOT_COUNT] = {
        "The Oregon Trail (1985)",
        "Where in the World is Carmen Sandiego?",
        "Karateka",
        "Lode Runner",
        "Prince of Persia (Disk 1)",
        "Ultima IV",
    };
    int ok = 1;
    for (int i = 0; i < CARTRIDGE_SLOT_COUNT; i++) {
        if (strcmp(cartridge_slots[i].title, expected_titles[i]) != 0) {
            fprintf(stderr, "FAIL: cartridge_slots[%d].title = \"%s\", expected \"%s\"\n",
                    i, cartridge_slots[i].title, expected_titles[i]);
            ok = 0;
        }
    }
    CHECK(ok, "test_slot_titles_match_brainstorm_order");
}

static void test_whole_table_fits_within_physical_reram(void) {
    uint32_t partition_offset_from_origin = CARTRIDGE_RERAM_BASE - CARTRIDGE_RERAM_ORIGIN;
    uint32_t available_for_slots = CARTRIDGE_RERAM_SIZE - partition_offset_from_origin;

    CHECK(CARTRIDGE_TOTAL_SIZE <= available_for_slots,
          "test_whole_table_fits_within_physical_reram: total slot bytes fit in remaining ReRAM");

    /* Sanity: the last slot's end address must not exceed ReRAM's top. */
    uint32_t reram_top = CARTRIDGE_RERAM_ORIGIN + CARTRIDGE_RERAM_SIZE;
    uint32_t last_slot_end = cartridge_slots[CARTRIDGE_SLOT_COUNT - 1].reram_addr + CARTRIDGE_SLOT_SIZE;
    CHECK(last_slot_end <= reram_top,
          "test_whole_table_fits_within_physical_reram: last slot ends at/before ReRAM top");
}

int main(void) {
    test_partition_base_is_exactly_2_5_mib_past_reram_origin();
    test_slot_size_matches_dos33_disk_image_size();
    test_slot_addresses_are_contiguous_with_no_gaps_or_overlap();
    test_slot_titles_match_brainstorm_order();
    test_whole_table_fits_within_physical_reram();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
