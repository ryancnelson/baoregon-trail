#ifndef CARTRIDGE_LAYOUT_H
#define CARTRIDGE_LAYOUT_H

#include <stdint.h>

/*
 * cartridge_layout.h -- multi-game "cartridge" ReRAM slot table, per
 * BRAINSTORM.md section 5.
 *
 * Partition boundary correction (2026-07-31, Discord #c-side hardware
 * findings from the dev who wrote the hardware-verified ReRAM driver):
 * keep the usable disk-image partition region starting around 2.5 MiB
 * into ReRAM, to leave ample headroom for .text/.rodata program code
 * growth as more BIO core / Dabao SDK integration code lands.
 *
 * BRAINSTORM.md's original worked example wrote this as "2.5 MB
 * (0x20080000)" -- that address is a math error: 0x20080000 is
 * 0x20000000 + 0x80000, and 0x80000 is only 512 KiB, not 2.5 MiB.
 * 2.5 MiB = 2.5 * 1024 * 1024 = 0x280000 bytes, so the corrected base
 * address is 0x20000000 + 0x280000 = CARTRIDGE_RERAM_BASE below. Caught
 * while wiring this table up against linker.ld's real 4 MiB ReRAM
 * region (0x20000000-0x203FFFFF, tools/check_linker_placement.py) --
 * verify the fixed arithmetic against real constants, not by eyeballing
 * a doc's hex literal.
 *
 * Each slot is a full 140 KiB DOS 3.3 disk image (DOS33_DISK_IMAGE_SIZE,
 * disk_sector_layout.h), addressed as an absolute ReRAM byte address
 * matching tools/embed_disk.py's output layout (each embedded .dsk_images
 * array can be placed at one of these offsets by the multi-game boot
 * selector, per BRAINSTORM.md section 5's boot-splash D-pad/button
 * selection flow).
 */

#define CARTRIDGE_RERAM_ORIGIN 0x20000000u
#define CARTRIDGE_RERAM_SIZE   0x00400000u /* 4.0 MiB, matches linker.ld's RERAM region */

/* 2.5 MiB into ReRAM -- see correction note above. */
#define CARTRIDGE_RERAM_BASE (CARTRIDGE_RERAM_ORIGIN + 0x00280000u)

/* 140 KiB per slot, matching DOS33_DISK_IMAGE_SIZE (disk_sector_layout.h)
 * exactly -- kept as a separate literal here (rather than #include-ing
 * disk_sector_layout.h) so this table has no compile-time dependency on
 * Duke's disk-layout internals; tests/test_cartridge_layout.c cross-checks
 * the two stay in sync. */
#define CARTRIDGE_SLOT_SIZE 143360u

#define CARTRIDGE_SLOT_COUNT 6

typedef struct {
    const char *title;
    uint32_t reram_addr;
} cartridge_slot_t;

/* Slot table, in the order listed in BRAINSTORM.md section 5. Offsets are
 * CARTRIDGE_RERAM_BASE + (index * CARTRIDGE_SLOT_SIZE) -- contiguous, no
 * gaps, so a 7th title could be appended without renumbering existing
 * slots (each slot's address is still base + index*size). */
extern const cartridge_slot_t cartridge_slots[CARTRIDGE_SLOT_COUNT];

/* Total bytes consumed by all cartridge slots. Used by
 * tests/test_cartridge_layout.c to confirm the table fits within
 * CARTRIDGE_RERAM_SIZE - (CARTRIDGE_RERAM_BASE - CARTRIDGE_RERAM_ORIGIN)
 * -- i.e. doesn't run off the end of the physical 4 MiB ReRAM. */
#define CARTRIDGE_TOTAL_SIZE ((uint32_t)CARTRIDGE_SLOT_COUNT * CARTRIDGE_SLOT_SIZE)

/* Safe accessor: returns pointer to slot at index, or NULL if index < 0 or index >= CARTRIDGE_SLOT_COUNT. */
const cartridge_slot_t *cartridge_layout_get_slot(int index);

#endif /* CARTRIDGE_LAYOUT_H */
