/*
 * main_renode_iox_test.c -- real, dedicated test firmware for the IOX
 * GPIO behavioral model (renode/iox_gpio.repl), per baoregon-trail
 * issue #2's IOX task.
 *
 * Confirms a real, working register-level round-trip: configures a
 * pin as input on IoxPort::PB (per the real register semantics in
 * xous-core's libs/bao1x-hal/src/iox.rs -- SFR_GPIOOE_CRGOE1 direction
 * register, bit=0 means input), then polls SFR_GPIOIN_SRGI1 (real
 * input-value register) in a loop and reports every transition it
 * observes to DUART. A companion .resc script
 * (renode/bao1x_iox_gpio_test.resc) pokes the SFR_GPIOIN_SRGI1
 * register mid-run to simulate an external button press -- this
 * firmware's job is to prove that transition is genuinely visible
 * through the real register layout, not just "the peripheral doesn't
 * crash".
 */
#include <stdint.h>

#define DUART_BASE 0x40042000u
#define DUART_TXD  (*(volatile uint32_t *)(DUART_BASE + 0x0000u))

#define IOX_BASE 0x5012f000u
/* Real offsets, per xous-core's bao1x_peri.svd (renode/bao1x_peri.svd) */
#define IOX_SFR_GPIOOE_CRGOE1 (*(volatile uint32_t *)(IOX_BASE + 0x14cu))  /* port PB direction */
#define IOX_SFR_GPIOIN_SRGI1  (*(volatile uint32_t *)(IOX_BASE + 0x17cu))  /* port PB input value */

/* Real IoxPort::PB pin index used for this test (matches
 * bao1x-hal-service's own convention of testing individual GPIO bits
 * -- pin 3 chosen arbitrarily as a representative "button" bit). */
#define TEST_PIN 3u

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
    duart_puts("baoregon-trail: IOX GPIO real register round-trip test (issue #2)\r\n");

    /* Configure TEST_PIN on port PB as an input, matching the real
     * driver semantics: IoxDir::Input == 0, so we CLEAR the direction
     * bit (real hardware default is already input/0 on reset, but do
     * it explicitly so this test doesn't depend on that assumption). */
    IOX_SFR_GPIOOE_CRGOE1 &= ~(1u << TEST_PIN);
    duart_puts("Configured PB pin ");
    duart_put_hex32(TEST_PIN);
    duart_puts(" as input (SFR_GPIOOE_CRGOE1 cleared)\r\n");

    uint32_t last_value = (IOX_SFR_GPIOIN_SRGI1 >> TEST_PIN) & 1u;
    duart_puts("Initial PB");
    duart_put_hex32(TEST_PIN);
    duart_puts(" state: ");
    duart_puts(last_value ? "HIGH" : "LOW");
    duart_puts("\r\n");
    duart_puts("Polling for a real transition (simulated button press)...\r\n");

    for (;;) {
        uint32_t value = (IOX_SFR_GPIOIN_SRGI1 >> TEST_PIN) & 1u;
        if (value != last_value) {
            duart_puts("TRANSITION: PB");
            duart_put_hex32(TEST_PIN);
            duart_puts(" ");
            duart_puts(last_value ? "HIGH" : "LOW");
            duart_puts(" -> ");
            duart_puts(value ? "HIGH" : "LOW");
            duart_puts(" (real SFR_GPIOIN_SRGI1 register read)\r\n");
            last_value = value;
        }
    }

    return 0;
}
