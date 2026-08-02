#include "emu_trace.h"

static emu_trace_putc_fn g_putc = 0;
static unsigned int g_heartbeat_counter = 0;

static void trace_puts(const char *s) {
    if (!g_putc) return;
    while (*s) g_putc((uint8_t)(*s++));
}

static void trace_put_hex8(uint8_t val) {
    static const char hex[] = "0123456789ABCDEF";
    if (!g_putc) return;
    g_putc((uint8_t)hex[(val >> 4) & 0xF]);
    g_putc((uint8_t)hex[val & 0xF]);
}

static void trace_put_hex16(uint16_t val) {
    trace_put_hex8((uint8_t)(val >> 8));
    trace_put_hex8((uint8_t)(val & 0xFF));
}

static void trace_put_hex32(uint32_t val) {
    trace_put_hex16((uint16_t)(val >> 16));
    trace_put_hex16((uint16_t)(val & 0xFFFF));
}

void emu_trace_init(emu_trace_putc_fn putc_fn) {
    g_putc = putc_fn;
    g_heartbeat_counter = 0;
}

void emu_trace_checkpoint(const char *msg) {
    if (!g_putc) return; /* safe no-op if never initialized */
    trace_puts(msg);
    g_putc((uint8_t)'\n');
}

void emu_trace_force_next_heartbeat(void) {
    /* Set counter to the threshold itself (not 0) so the VERY NEXT
     * emu_trace_heartbeat() call -- which increments before comparing
     * -- reaches EMU_TRACE_HEARTBEAT_INTERVAL and actually prints,
     * rather than needing one more full interval to elapse first. */
    g_heartbeat_counter = EMU_TRACE_HEARTBEAT_INTERVAL - 1u;
}

void emu_trace_heartbeat(uint16_t pc, uint8_t a, uint8_t x, uint8_t y,
                          uint8_t sp, uint32_t clockticks) {
    if (!g_putc) return; /* safe no-op if never initialized */

    g_heartbeat_counter++;
    if (g_heartbeat_counter < EMU_TRACE_HEARTBEAT_INTERVAL) {
        return;
    }
    g_heartbeat_counter = 0;

    trace_puts("hb pc=");
    trace_put_hex16(pc);
    trace_puts(" a=");
    trace_put_hex8(a);
    trace_puts(" x=");
    trace_put_hex8(x);
    trace_puts(" y=");
    trace_put_hex8(y);
    trace_puts(" sp=");
    trace_put_hex8(sp);
    trace_puts(" cyc=");
    trace_put_hex32(clockticks);
    g_putc((uint8_t)'\n');
}
