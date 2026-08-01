/*
 * RED test: end-to-end pipeline proof -- bytes written into a cartridge
 * slot via rram_driver.c's rram_write() (the real ReRAM-flashing write
 * path, bounds-checked against CARTRIDGE_RERAM_BASE) are correctly read
 * back through disk_trap.c's sector-oriented interface (the same
 * disk_trap_select_sector()/disk_trap_read_byte() calls apple2_mem.c's
 * $C0E0-$C0EF soft-switch trap uses during real 6502 disk I/O).
 *
 * Until now, tests/test_rram_cartridge_integration.c only proved
 * rram_read()/rram_write() round-trip through rram_driver.c's OWN read
 * path, and tests/test_disk_trap.c only proved disk_trap.c's sector math
 * against a directly-injected in-memory image (not one written via the
 * ReRAM driver at all). Nothing proved the two halves of the real
 * pipeline -- "flash a game via the ReRAM driver" and "read it back via
 * the disk trap the emulated 6502 actually uses" -- connect correctly.
 * This closes that gap.
 */
#include <stdio.h>
#include <string.h>
#include "../src/rram_driver.h"
#include "../src/cartridge_layout.h"
#include "../src/disk_sector_layout.h"
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

static uint8_t g_cartridge_store[CARTRIDGE_TOTAL_SIZE];

/* Real XIP hardware: cartridge_slots[n].reram_addr is a directly
 * dereferenceable ReRAM address, so disk_trap_set_image() is handed that
 * address cast to a pointer as-is (see boot_splash.c's
 * (const uint8_t *)(uintptr_t)slot->reram_addr pattern). On host, the
 * backing store is a plain array starting at CARTRIDGE_RERAM_BASE (not
 * really mapped at that address), so the equivalent pointer is the store
 * base plus the slot's offset from CARTRIDGE_RERAM_BASE -- exactly what
 * rram_driver_attach_cartridge_partition() itself assumes internally. */
static const uint8_t *host_pointer_for_slot(int slot_index) {
    uint32_t offset_from_base = cartridge_slots[slot_index].reram_addr - CARTRIDGE_RERAM_BASE;
    return g_cartridge_store + offset_from_base;
}

static void test_flash_via_rram_write_then_read_via_disk_trap_matches(void) {
    memset(g_cartridge_store, 0, sizeof(g_cartridge_store));
    rram_driver_attach_cartridge_partition(g_cartridge_store);

    /* "Flash" a synthetic DOS 3.3 disk image into slot 2 (Karateka) via
     * the real ReRAM write path, byte pattern distinct-within-a-sector
     * so any off-by-one/wrong-sector read would be immediately visible. */
    const int slot_index = 2;
    static uint8_t game_image[DOS33_DISK_IMAGE_SIZE];
    for (uint32_t i = 0; i < DOS33_DISK_IMAGE_SIZE; i++) {
        game_image[i] = (uint8_t)(i & 0xFF);
    }

    int rc = rram_write(cartridge_slots[slot_index].reram_addr, game_image, DOS33_DISK_IMAGE_SIZE);
    CHECK(rc == RRAM_OK, "test_flash_via_rram_write_then_read_via_disk_trap_matches: rram_write into slot 2 succeeds");

    /* Now read it back via disk_trap.c's sector interface -- the same
     * calls apple2_mem.c's $C0E0/$C0E1/$C0EC soft-switch trap makes
     * during real 6502 disk I/O. This is the pointer a real
     * boot_splash_handle_button() SELECT would hand to
     * disk_trap_set_image() for this slot. */
    disk_trap_set_image(host_pointer_for_slot(slot_index));

    /* Track 17, sector 0 -- DOS 3.3's VTOC, a meaningful non-zero
     * reference point (matches the convention used in
     * tests/test_disk_trap.c). */
    int select_rc = disk_trap_select_sector(17, 0);
    CHECK(select_rc == 0, "test_flash_via_rram_write_then_read_via_disk_trap_matches: disk_trap_select_sector(17, 0) succeeds");

    uint32_t expected_base_offset;
    dos33_sector_offset(17, 0, &expected_base_offset);

    int bytes_ok = 1;
    for (int i = 0; i < 256; i++) {
        uint8_t got = disk_trap_read_byte((uint8_t)i);
        uint8_t expected = game_image[expected_base_offset + i];
        if (got != expected) {
            fprintf(stderr,
                    "FAIL: track 17 sector 0 byte %d = 0x%02X, expected 0x%02X "
                    "(flashed-via-rram_write value did not survive to disk_trap read)\n",
                    i, got, expected);
            bytes_ok = 0;
            break;
        }
    }
    CHECK(bytes_ok, "test_flash_via_rram_write_then_read_via_disk_trap_matches: all 256 bytes of track 17 sector 0 match");
}

static void test_two_different_slots_dont_cross_contaminate_through_the_pipeline(void) {
    /* Flash distinct images into two different slots via rram_write(),
     * then confirm disk_trap.c reads back the CORRECT slot's data when
     * pointed at each -- proves cartridge_slots[] offsets, rram_driver's
     * bounds math, and disk_trap's sector math all agree on where each
     * slot actually lives, not just that any one slot alone round-trips. */
    memset(g_cartridge_store, 0, sizeof(g_cartridge_store));
    rram_driver_attach_cartridge_partition(g_cartridge_store);

    static uint8_t image_a[DOS33_DISK_IMAGE_SIZE];
    static uint8_t image_b[DOS33_DISK_IMAGE_SIZE];
    memset(image_a, 0xAA, sizeof(image_a));
    memset(image_b, 0xBB, sizeof(image_b));

    rram_write(cartridge_slots[0].reram_addr, image_a, DOS33_DISK_IMAGE_SIZE); /* Oregon Trail */
    rram_write(cartridge_slots[5].reram_addr, image_b, DOS33_DISK_IMAGE_SIZE); /* Ultima IV */

    /* Point disk_trap at slot 0, read a sector, must be all 0xAA. */
    disk_trap_set_image(host_pointer_for_slot(0));
    disk_trap_select_sector(0, 0);
    uint8_t byte_from_slot0 = disk_trap_read_byte(0);
    CHECK(byte_from_slot0 == 0xAA,
          "test_two_different_slots_dont_cross_contaminate_through_the_pipeline: slot 0 reads 0xAA");

    /* Point disk_trap at slot 5, same track/sector, must be all 0xBB --
     * not leftover 0xAA from slot 0 and not garbage. */
    disk_trap_set_image(host_pointer_for_slot(5));
    disk_trap_select_sector(0, 0);
    uint8_t byte_from_slot5 = disk_trap_read_byte(0);
    CHECK(byte_from_slot5 == 0xBB,
          "test_two_different_slots_dont_cross_contaminate_through_the_pipeline: slot 5 reads 0xBB, not slot 0's data");
}

int main(void) {
    test_flash_via_rram_write_then_read_via_disk_trap_matches();
    test_two_different_slots_dont_cross_contaminate_through_the_pipeline();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
