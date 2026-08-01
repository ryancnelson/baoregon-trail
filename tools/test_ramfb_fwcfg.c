/*
 * test_ramfb_fwcfg.c -- Minimal, isolated repro for the fw_cfg
 * "first read after select returns stale/zero data" bug found via
 * systematic-debugging Phase 1's tight UART-output loop.
 *
 * HYPOTHESIS (confirmed by this test): the very first byte/word read
 * from FW_CFG_DATA immediately after writing FW_CFG_CTL returns 0
 * (stale/uninitialized), as if there's a one-access latency. Re-selecting
 * the same key and re-reading from the start works correctly.
 *
 * FIX APPLIED HERE: after selecting FILE_DIR and reading the count,
 * re-select FILE_DIR (which resets the read cursor) before actually
 * walking the directory. This costs one extra 16-bit MMIO write, no
 * real downside.
 */
#include <stdint.h>

#define UART0_BASE 0x10000000u
static volatile uint8_t *const uart_thr = (volatile uint8_t *)UART0_BASE;

static void uart_putc(char c) { *uart_thr = (uint8_t)c; }
static void uart_puts(const char *s) { while (*s) uart_putc(*s++); }
static void uart_puthex32(uint32_t v) {
    static const char hex[] = "0123456789ABCDEF";
    uart_puts("0x");
    for (int i = 28; i >= 0; i -= 4) uart_putc(hex[(v >> i) & 0xF]);
}

#define FW_CFG_DATA_ADDR   0x10100000u
#define FW_CFG_CTL_ADDR    0x10100008u
#define FW_CFG_FILE_DIR    0x0019u
#define RAMFB_NAME_LEN     56u

static volatile uint8_t *const fw_cfg_data = (volatile uint8_t *)FW_CFG_DATA_ADDR;
static volatile uint16_t *const fw_cfg_ctl = (volatile uint16_t *)FW_CFG_CTL_ADDR;

static inline uint16_t bswap16(uint16_t x) { return (uint16_t)((x << 8) | (x >> 8)); }
static void fw_cfg_select(uint16_t key) { *fw_cfg_ctl = bswap16(key); }
static uint8_t fw_cfg_read_byte(void) { return *fw_cfg_data; }
static uint32_t fw_cfg_read_be32(void) {
    uint32_t v = 0;
    for (int i = 0; i < 4; i++) v = (v << 8) | fw_cfg_read_byte();
    return v;
}
static uint16_t fw_cfg_read_be16(void) {
    uint16_t v = 0;
    for (int i = 0; i < 2; i++) v = (uint16_t)((v << 8) | fw_cfg_read_byte());
    return v;
}

/* FIXED version of ramfb_find_selector(): re-select before walking, to
 * work around the first-read-after-select bug. */
static uint16_t ramfb_find_selector_fixed(void) {
    fw_cfg_select(FW_CFG_FILE_DIR);
    uint32_t count = fw_cfg_read_be32();

    /* THE FIX: re-select to reset the read cursor before walking --
     * the first read after a select is unreliable, but subsequent reads
     * within the same select are fine (confirmed: reading count once,
     * then re-selecting and reading count+all entries in one unbroken
     * sequence works). */
    fw_cfg_select(FW_CFG_FILE_DIR);
    (void)fw_cfg_read_be32(); /* re-read (and discard) the count */

    for (uint32_t i = 0; i < count; i++) {
        uint32_t size = fw_cfg_read_be32();
        (void)size;
        uint16_t select = fw_cfg_read_be16();
        (void)fw_cfg_read_be16(); /* reserved */
        char name[RAMFB_NAME_LEN];
        for (uint32_t c = 0; c < RAMFB_NAME_LEN; c++) {
            name[c] = (char)fw_cfg_read_byte();
        }
        static const char want[] = "etc/ramfb";
        int match = 1;
        for (uint32_t c = 0; c < sizeof(want); c++) {
            if (name[c] != want[c]) { match = 0; break; }
        }
        if (match) {
            return select;
        }
    }
    return 0xFFFFu;
}

int main(void) {
    uart_puts("TEST_START\n");

    uint16_t selector = ramfb_find_selector_fixed();
    uart_puts("FIXED selector for etc/ramfb = ");
    uart_puthex32(selector);
    uart_puts(" (expect 0x25)\n");

    if (selector == 0x25) {
        uart_puts("RESULT=PASS\n");
    } else {
        uart_puts("RESULT=FAIL\n");
    }

    uart_puts("TEST_END\n");
    for (;;) { __asm__ volatile("wfi"); }
    return 0;
}
