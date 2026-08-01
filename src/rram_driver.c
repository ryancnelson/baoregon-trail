/*
 * rram_driver.c -- host-testable port of armstrongsubero/dabao-sdk's
 * hardware-verified ReRAM driver algorithm. See rram_driver.h for the
 * design rationale (real driver: src/bao1x/hardware_rram/rram.c).
 */
#include "rram_driver.h"
#include <string.h>

static uint8_t *g_store = 0;
static uint32_t g_base_addr = 0;
static uint32_t g_size = 0;

void rram_driver_attach_backing_store(uint8_t *store, uint32_t base_addr, uint32_t size) {
    g_store = store;
    g_base_addr = base_addr;
    g_size = size;
}

static int addr_in_bounds(uint32_t addr, uint32_t len) {
    if (g_store == 0) {
        return 0;
    }
    if (addr < g_base_addr) {
        return 0;
    }
    /* Use 64-bit intermediate to avoid overflow on addr+len near UINT32_MAX,
     * matching the spirit of the real driver's bounds checks but guarding
     * against a wraparound bug the original didn't need to worry about at
     * these test-scale addresses. */
    uint64_t end = (uint64_t)addr + (uint64_t)len;
    uint64_t region_end = (uint64_t)g_base_addr + (uint64_t)g_size;
    return end <= region_end;
}

void rram_read(uint32_t addr, uint8_t *buf, uint32_t len) {
    if (g_store == 0 || buf == 0 || len == 0) {
        return;
    }
    uint32_t offset = addr - g_base_addr;
    memcpy(buf, g_store + offset, len);
}

static void write_page_raw(uint32_t page_addr, const uint8_t *data) {
    uint32_t offset = page_addr - g_base_addr;
    memcpy(g_store + offset, data, RRAM_PAGE_SIZE);
}

static int verify_page(uint32_t page_addr, const uint8_t *expected) {
    uint32_t offset = page_addr - g_base_addr;
    return memcmp(g_store + offset, expected, RRAM_PAGE_SIZE) == 0 ? 0 : -1;
}

int rram_write_page(uint32_t page_addr, const uint8_t *data) {
    if (data == 0) {
        return RRAM_ERR_NULL_BUFFER;
    }
    if (!addr_in_bounds(page_addr, RRAM_PAGE_SIZE)) {
        return RRAM_ERR_OUT_OF_BOUNDS;
    }
    if (page_addr & (RRAM_PAGE_SIZE - 1)) {
        return RRAM_ERR_OUT_OF_BOUNDS; /* not page-aligned */
    }

    /* Write with retry-on-verify-failure, matching dabao-sdk's
     * RRAM_WRITE_RETRIES == 2 total attempts. On a plain in-memory
     * backing store the write always succeeds on the first attempt (no
     * real hardware flakiness to retry around), but the retry structure
     * is preserved so the algorithm matches the hardware-verified driver
     * exactly -- swapping in real MMIO writes later is a drop-in
     * replacement of write_page_raw()'s body, not a rewrite of this
     * control flow. */
    const int RRAM_WRITE_RETRIES = 2;
    for (int attempt = 0; attempt < RRAM_WRITE_RETRIES; attempt++) {
        write_page_raw(page_addr, data);
        if (verify_page(page_addr, data) == 0) {
            return RRAM_OK;
        }
    }
    return RRAM_ERR_VERIFY_FAILED;
}

int rram_erase_page(uint32_t page_addr) {
    uint8_t ff[RRAM_PAGE_SIZE];
    for (uint32_t i = 0; i < RRAM_PAGE_SIZE; i++) {
        ff[i] = 0xFF;
    }
    return rram_write_page(page_addr, ff);
}

int rram_write(uint32_t addr, const uint8_t *data, uint32_t len) {
    if (len == 0) {
        return RRAM_OK;
    }
    if (data == 0) {
        return RRAM_ERR_NULL_BUFFER;
    }
    if (!addr_in_bounds(addr, len)) {
        return RRAM_ERR_OUT_OF_BOUNDS;
    }

    uint8_t page_buf[RRAM_PAGE_SIZE];

    /* Ragged start: unaligned beginning. */
    uint32_t offset_in_page = addr & (RRAM_PAGE_SIZE - 1);
    if (offset_in_page != 0) {
        uint32_t page_start = addr & ~(RRAM_PAGE_SIZE - 1);
        uint32_t copy_len = RRAM_PAGE_SIZE - offset_in_page;
        if (copy_len > len) {
            copy_len = len;
        }

        rram_read(page_start, page_buf, RRAM_PAGE_SIZE);
        for (uint32_t i = 0; i < copy_len; i++) {
            page_buf[offset_in_page + i] = data[i];
        }

        int rc = rram_write_page(page_start, page_buf);
        if (rc != RRAM_OK) {
            return rc;
        }

        data += copy_len;
        addr += copy_len;
        len -= copy_len;
    }

    /* Full aligned pages. */
    while (len >= RRAM_PAGE_SIZE) {
        int rc = rram_write_page(addr, data);
        if (rc != RRAM_OK) {
            return rc;
        }
        data += RRAM_PAGE_SIZE;
        addr += RRAM_PAGE_SIZE;
        len -= RRAM_PAGE_SIZE;
    }

    /* Ragged end. */
    if (len > 0) {
        rram_read(addr, page_buf, RRAM_PAGE_SIZE);
        for (uint32_t i = 0; i < len; i++) {
            page_buf[i] = data[i];
        }
        int rc = rram_write_page(addr, page_buf);
        if (rc != RRAM_OK) {
            return rc;
        }
    }

    return RRAM_OK;
}
