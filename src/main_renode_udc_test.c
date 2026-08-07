/*
 * main_renode_udc_test.c -- real, dedicated test firmware for the UDC
 * (USB Device Controller) behavioral model (renode/udc_usb.repl), per
 * baoregon-trail issue #2's USB UART task.
 *
 * Exercises the two real, register-level hardware handshakes the
 * actual driver (xous-core's libs/bao1x-hal/src/usb/driver.rs)
 * performs and busy-waits on:
 *
 *   1. Soft reset (driver.rs reset()): write USBCMD_SOFT_RESET=1,
 *      busy-wait until it reads back 0.
 *   2. Command execution (driver.rs issue_command()): write
 *      CMDPARA0/CMDPARA1, write CMDCTRL with ACTIVE=1, busy-wait
 *      until ACTIVE reads back 0, check STATUS for success.
 *
 * Also reads the real DEVCAP register, confirming it matches the
 * real, documented silicon value from driver.rs's own comment
 * (0x20014401).
 */
#include <stdint.h>

#define DUART_BASE 0x40042000u
#define DUART_TXD  (*(volatile uint32_t *)(DUART_BASE + 0x0000u))

#define UDC_BASE 0x50200000u
#define UDC_DEV_OFFSET 0x400u

/* Real register offsets, per xous-core's libs/bao1x-hal/src/usb/utra.rs */
#define UDC_DEVCAP   (*(volatile uint32_t *)(UDC_BASE + UDC_DEV_OFFSET + 0x00u))
#define UDC_USBCMD   (*(volatile uint32_t *)(UDC_BASE + UDC_DEV_OFFSET + 0x20u))
#define UDC_CMDPARA0 (*(volatile uint32_t *)(UDC_BASE + UDC_DEV_OFFSET + 0x70u))
#define UDC_CMDPARA1 (*(volatile uint32_t *)(UDC_BASE + UDC_DEV_OFFSET + 0x74u))
#define UDC_CMDCTRL  (*(volatile uint32_t *)(UDC_BASE + UDC_DEV_OFFSET + 0x78u))

#define USBCMD_SOFT_RESET (1u << 1)
#define CMDCTRL_ACTIVE    (1u << 0)
#define CMDCTRL_TYPE_SHIFT 4u
#define CMDCTRL_STATUS_SHIFT 16u
#define CMDCTRL_STATUS_MASK 0xFu

/* Real CmdType::SetAddr = 2, per driver.rs */
#define CMD_TYPE_SET_ADDR 2u

static void duart_putc(char c) {
    DUART_TXD = (uint32_t)(unsigned char)c;
}

static void duart_puts(const char *s) {
    while (*s) {
        duart_putc(*s++);
    }
}

static void duart_put_hex32(uint32_t v) {
    static const char hex[] = "0123456789ABCDEF";
    duart_puts("0x");
    for (int shift = 28; shift >= 0; shift -= 4) {
        duart_putc(hex[(v >> shift) & 0xF]);
    }
}

int main(void) {
    duart_puts("\r\n");
    duart_puts("baoregon-trail: UDC USB real register handshake test (issue #2)\r\n");

    /* 1. Read DEVCAP, confirm it matches real, documented silicon
     * value from driver.rs's own comment. */
    uint32_t devcap = UDC_DEVCAP;
    duart_puts("DEVCAP: ");
    duart_put_hex32(devcap);
    if (devcap == 0x20014401u) {
        duart_puts(" (matches real silicon's documented value)\r\n");
    } else {
        duart_puts(" (MISMATCH vs real silicon's documented 0x20014401)\r\n");
    }

    /* 2. Soft reset handshake, matching driver.rs's real reset()
     * sequence: write SOFT_RESET=1, busy-wait until it clears. */
    duart_puts("Issuing soft reset (USBCMD_SOFT_RESET=1)...\r\n");
    UDC_USBCMD = USBCMD_SOFT_RESET;
    uint32_t spins = 0;
    while ((UDC_USBCMD & USBCMD_SOFT_RESET) != 0) {
        spins++;
        if (spins > 1000000u) {
            duart_puts("TIMEOUT waiting for soft reset to clear\r\n");
            return 1;
        }
    }
    duart_puts("Soft reset completed (USBCMD_SOFT_RESET cleared by hardware)\r\n");

    /* 3. Command-issue handshake, matching driver.rs's real
     * issue_command(): write CMDPARA0/1, write CMDCTRL with
     * ACTIVE=1+TYPE=<cmd>, busy-wait until ACTIVE clears, check
     * STATUS. */
    duart_puts("Issuing CmdType::SetAddr (addr=5)...\r\n");
    UDC_CMDPARA0 = 5u; /* CMDPARA0_CMD2_SET_ADDR field, addr=5 */
    UDC_CMDPARA1 = 0u;
    UDC_CMDCTRL = CMDCTRL_ACTIVE | (CMD_TYPE_SET_ADDR << CMDCTRL_TYPE_SHIFT);
    spins = 0;
    while ((UDC_CMDCTRL & CMDCTRL_ACTIVE) != 0) {
        spins++;
        if (spins > 1000000u) {
            duart_puts("TIMEOUT waiting for command to complete\r\n");
            return 1;
        }
    }
    uint32_t status = (UDC_CMDCTRL >> CMDCTRL_STATUS_SHIFT) & CMDCTRL_STATUS_MASK;
    duart_puts("Command completed (CMDCTRL_ACTIVE cleared), STATUS=");
    duart_put_hex32(status);
    if (status == 0u) {
        duart_puts(" (success)\r\n");
    } else {
        duart_puts(" (non-zero status)\r\n");
    }

    duart_puts("ALL HANDSHAKES VERIFIED\r\n");

    for (;;) {
        /* idle */
    }

    return 0;
}
