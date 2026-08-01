#ifndef RRAM_DRIVER_H
#define RRAM_DRIVER_H

#include <stdint.h>

/*
 * rram_driver.c -- ReRAM (RRAM) storage driver, ported from
 * armstrongsubero/dabao-sdk's hardware-verified src/bao1x/hardware_rram/rram.c
 * (referenced 2026-07-31 per Discord #c-side hardware findings).
 *
 * Real Baochip-1x hardware requires unaligned-address handling (read-
 * modify-write on partial pages) and readback verification after every
 * page write -- this is NOT optional per the dev who wrote the
 * hardware-verified driver. This module is a host-testable port of that
 * logic: same page-write/read-modify-write/verify structure, but backed
 * by a plain in-memory buffer (rram_driver_attach_backing_store()) instead
 * of real MMIO RRC-controller register writes, so it can be RED-GREEN
 * tested without hardware. The RRC command-sequence + cache-flush parts
 * of the real driver (RRC_LOAD_BUFFER/RRC_WRITE_BUFFER, fence.i) are
 * hardware-specific and are a follow-up once Dabao SDK integration lands
 * (NEXT_STEPS.md Step 5) -- this module proves the *algorithm*
 * (bounds checking, page alignment, ragged start/end splitting, retry-
 * on-verify-failure) is correct first, independent of the MMIO layer.
 *
 * Page size: 32 bytes, matching the real hardware's RRC page granularity
 * (armstrongsubero/dabao-sdk's RRAM_PAGE_SIZE).
 *
 * Partition note (2026-07-31, Discord #c-side hardware guidance): the
 * usable ReRAM disk-image partition region should start around 2.5 MiB
 * into ReRAM to leave ample headroom for .text/.rodata program code
 * growth. See disk_sector_layout.h / BRAINSTORM.md section 5 for the
 * corrected offset -- BRAINSTORM.md's original example address
 * (0x20080000) was only 512 KiB in, not 2.5 MiB; the corrected offset is
 * 0x20280000 (0x20000000 + 0x280000, where 0x280000 == 2.5 MiB).
 */

#define RRAM_PAGE_SIZE 32u

typedef enum {
    RRAM_OK = 0,
    RRAM_ERR_OUT_OF_BOUNDS = -1,
    RRAM_ERR_VERIFY_FAILED = -2,
    RRAM_ERR_NULL_BUFFER = -3,
} rram_status_t;

/*
 * Attach the in-memory backing store this driver operates against, plus
 * the [base, base+size) address range considered valid for writes/reads.
 * Mirrors real hardware's RRAM_USER_BASE/RRAM_USER_TOP bounds-checking
 * (dabao-sdk restricts writes to the upper region to avoid bricking the
 * boot sequence -- this driver applies the same bounds-checked-write
 * discipline against whatever region the caller designates as "user").
 *
 * store must remain valid for the lifetime of subsequent calls. addr
 * values passed to rram_read()/rram_write() are absolute (relative to
 * base_addr), matching the real driver's absolute-RRAM-address API.
 */
void rram_driver_attach_backing_store(uint8_t *store, uint32_t base_addr, uint32_t size);

/*
 * Read len bytes starting at absolute address addr into buf. Always
 * succeeds if the whole range [addr, addr+len) is within the attached
 * backing store's bounds; out-of-bounds reads are NOT range-checked here
 * (matching the real driver, which treats rram_read() as a plain memcpy
 * with no bounds enforcement -- only writes are bounds-checked, per
 * dabao-sdk's design to prevent bricking the boot sequence on write, not
 * read).
 */
void rram_read(uint32_t addr, uint8_t *buf, uint32_t len);

/*
 * Write len bytes from data to absolute address addr. Handles unaligned
 * addr and arbitrary len via read-modify-write on partial (ragged)
 * leading/trailing pages, exactly like the real driver's rram_write():
 *   1. ragged start: read the page containing addr, patch in the new
 *      bytes for the unaligned portion, write the whole page back.
 *   2. full aligned pages: write each RRAM_PAGE_SIZE-byte chunk directly.
 *   3. ragged end: read-modify-write the final partial page.
 *
 * Every page write is verified by readback (rram_write_page()); if
 * verification fails, the whole rram_write() call fails at that point
 * (does not attempt to continue writing further pages with a known-bad
 * page behind it).
 *
 * Returns RRAM_OK on success, RRAM_ERR_OUT_OF_BOUNDS if [addr, addr+len)
 * falls outside the attached backing store's bounds, RRAM_ERR_NULL_BUFFER
 * if data is NULL (and len > 0), or RRAM_ERR_VERIFY_FAILED if a page
 * failed verification after retries.
 */
int rram_write(uint32_t addr, const uint8_t *data, uint32_t len);

/*
 * Write and verify a single RRAM_PAGE_SIZE-byte page. page_addr must be
 * page-aligned and within the attached backing store's bounds. Retries
 * once on verify failure before giving up (matching dabao-sdk's
 * RRAM_WRITE_RETRIES == 2 total attempts).
 *
 * Returns RRAM_OK, RRAM_ERR_OUT_OF_BOUNDS (bad address/alignment), or
 * RRAM_ERR_VERIFY_FAILED (readback mismatch after retries).
 */
int rram_write_page(uint32_t page_addr, const uint8_t *data);

/*
 * Erase a page by writing all 0xFF, for EEPROM-style usage patterns.
 * ReRAM does not require a separate erase step before writing (unlike
 * flash), but this is provided for API parity with the real driver.
 */
int rram_erase_page(uint32_t page_addr);

#endif /* RRAM_DRIVER_H */
