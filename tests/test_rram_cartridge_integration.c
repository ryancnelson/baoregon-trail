/*
 * RED test: rram_driver.c integration with cartridge_layout.h's
 * CARTRIDGE_RERAM_BASE / CARTRIDGE_TOTAL_SIZE, so that disk sector writes
 * through rram_write()/rram_write_page() are strictly bounds-checked
 * against the cartridge partition -- not an arbitrary caller-supplied
 * region, and specifically NOT allowed to write into the .text/.rodata
 * headroom below CARTRIDGE_RERAM_BASE (the whole point of the 2.5 MiB
 * boundary correction).
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

/* Synthetic backing store sized to the real cartridge partition
 * (CARTRIDGE_TOTAL_SIZE == 6 * 143360 == ~840 KiB) so real cartridge
 * addresses can be exercised directly, not scaled-down surrogates. */
static uint8_t g_cartridge_store[CARTRIDGE_TOTAL_SIZE];

static void test_attach_cartridge_partition_uses_correct_base_and_size(void) {
    memset(g_cartridge_store, 0, sizeof(g_cartridge_store));
    rram_driver_attach_cartridge_partition(g_cartridge_store);

    /* Write and read back at the very first byte of the cartridge
     * partition (CARTRIDGE_RERAM_BASE == cartridge_slots[0].reram_addr) --
     * proves the driver is actually anchored at the corrected 2.5 MiB
     * offset, not some other base. */
    uint8_t page[RRAM_PAGE_SIZE];
    for (int i = 0; i < (int)RRAM_PAGE_SIZE; i++) {
        page[i] = (uint8_t)(0x10 + i);
    }
    int rc = rram_write_page(CARTRIDGE_RERAM_BASE, page);
    CHECK(rc == RRAM_OK, "test_attach_cartridge_partition_uses_correct_base_and_size: write at CARTRIDGE_RERAM_BASE succeeds");

    uint8_t readback[RRAM_PAGE_SIZE];
    rram_read(CARTRIDGE_RERAM_BASE, readback, RRAM_PAGE_SIZE);
    CHECK(memcmp(readback, page, RRAM_PAGE_SIZE) == 0,
          "test_attach_cartridge_partition_uses_correct_base_and_size: readback matches");
}

static void test_write_below_cartridge_base_is_rejected(void) {
    /* This is the entire point of the 2.5 MiB correction: writes must
     * NEVER land in the .text/.rodata headroom below CARTRIDGE_RERAM_BASE.
     * A disk-image write bug that wandered a page below the partition
     * boundary would corrupt program code on real hardware -- this proves
     * the driver refuses that categorically, not just "usually". */
    memset(g_cartridge_store, 0, sizeof(g_cartridge_store));
    rram_driver_attach_cartridge_partition(g_cartridge_store);

    uint8_t page[RRAM_PAGE_SIZE] = {0};
    int rc = rram_write_page(CARTRIDGE_RERAM_BASE - RRAM_PAGE_SIZE, page);
    CHECK(rc == RRAM_ERR_OUT_OF_BOUNDS,
          "test_write_below_cartridge_base_is_rejected");
}

static void test_write_at_last_valid_page_of_partition_succeeds(void) {
    memset(g_cartridge_store, 0, sizeof(g_cartridge_store));
    rram_driver_attach_cartridge_partition(g_cartridge_store);

    uint32_t last_page_addr = CARTRIDGE_RERAM_BASE + CARTRIDGE_TOTAL_SIZE - RRAM_PAGE_SIZE;
    uint8_t page[RRAM_PAGE_SIZE];
    for (int i = 0; i < (int)RRAM_PAGE_SIZE; i++) {
        page[i] = (uint8_t)(0xA0 + i);
    }
    int rc = rram_write_page(last_page_addr, page);
    CHECK(rc == RRAM_OK,
          "test_write_at_last_valid_page_of_partition_succeeds");
}

static void test_write_past_end_of_cartridge_partition_is_rejected(void) {
    /* One page past the very end of CARTRIDGE_TOTAL_SIZE -- must be
     * rejected, not silently written into whatever memory happens to
     * follow the backing store on real hardware. */
    memset(g_cartridge_store, 0, sizeof(g_cartridge_store));
    rram_driver_attach_cartridge_partition(g_cartridge_store);

    uint32_t past_end_addr = CARTRIDGE_RERAM_BASE + CARTRIDGE_TOTAL_SIZE;
    uint8_t page[RRAM_PAGE_SIZE] = {0};
    int rc = rram_write_page(past_end_addr, page);
    CHECK(rc == RRAM_ERR_OUT_OF_BOUNDS,
          "test_write_past_end_of_cartridge_partition_is_rejected");
}

static void test_write_full_disk_image_into_each_cartridge_slot(void) {
    /* Integration proof: write a full-size (CARTRIDGE_SLOT_SIZE-byte)
     * disk image into every cartridge_slots[] address via rram_write()
     * (the unaligned/arbitrary-length path, not just rram_write_page()),
     * and confirm each slot's content is independently correct and
     * doesn't bleed into its neighbors. Mirrors the real embed_disk.py ->
     * ReRAM flashing flow this driver ultimately supports. */
    memset(g_cartridge_store, 0, sizeof(g_cartridge_store));
    rram_driver_attach_cartridge_partition(g_cartridge_store);

    static uint8_t slot_image[CARTRIDGE_SLOT_SIZE];
    int ok = 1;

    for (int slot = 0; slot < CARTRIDGE_SLOT_COUNT; slot++) {
        /* Each slot gets a distinct fill byte so cross-slot bleed would
         * be immediately visible on readback. */
        uint8_t fill = (uint8_t)(0x50 + slot);
        memset(slot_image, fill, sizeof(slot_image));

        int rc = rram_write(cartridge_slots[slot].reram_addr, slot_image, CARTRIDGE_SLOT_SIZE);
        if (rc != RRAM_OK) {
            fprintf(stderr, "FAIL: rram_write into slot %d (%s) failed, rc=%d\n",
                    slot, cartridge_slots[slot].title, rc);
            ok = 0;
        }
    }

    if (ok) {
        for (int slot = 0; slot < CARTRIDGE_SLOT_COUNT; slot++) {
            uint8_t expected_fill = (uint8_t)(0x50 + slot);
            uint8_t readback_buf[CARTRIDGE_SLOT_SIZE];
            rram_read(cartridge_slots[slot].reram_addr, readback_buf, CARTRIDGE_SLOT_SIZE);

            for (uint32_t i = 0; i < CARTRIDGE_SLOT_SIZE; i++) {
                if (readback_buf[i] != expected_fill) {
                    fprintf(stderr,
                            "FAIL: slot %d (%s) byte %u = 0x%02X, expected 0x%02X "
                            "(cross-slot bleed or write bug)\n",
                            slot, cartridge_slots[slot].title, i, readback_buf[i], expected_fill);
                    ok = 0;
                    break;
                }
            }
        }
    }

    CHECK(ok, "test_write_full_disk_image_into_each_cartridge_slot");
}

int main(void) {
    test_attach_cartridge_partition_uses_correct_base_and_size();
    test_write_below_cartridge_base_is_rejected();
    test_write_at_last_valid_page_of_partition_succeeds();
    test_write_past_end_of_cartridge_partition_is_rejected();
    test_write_full_disk_image_into_each_cartridge_slot();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
