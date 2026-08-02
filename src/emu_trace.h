/*
 * emu_trace.h -- first-class observability/tracing for the 6502
 * emulator core, meant to be reused across every QEMU boot demo and
 * (eventually) real Baochip-1x hardware bring-up.
 *
 * WHY THIS EXISTS: this project is going to be the base platform for
 * DEF CON badge hacking all next week. Badge hackers will constantly
 * hit "is my code doing anything, or is it stuck?" with no way to
 * dtrace/ptrace/gdb-attach into a from-scratch software 6502 core
 * running inside a from-scratch RISC-V program -- there's no OS, no
 * debugger, nothing but whatever this emulator chooses to expose. This
 * module is the answer: a small, freestanding-safe, zero-libc tracing
 * helper that any entry point can wire up in a few lines to get real
 * "is it alive, and what's it doing" visibility over UART (or any other
 * byte-sink -- see emu_trace_init()'s function-pointer contract, same
 * pattern as uart_keyboard_bridge.h).
 *
 * USAGE (typical boot-loop wiring):
 *   emu_trace_init(my_putc_fn);
 *   emu_trace_checkpoint("boot: entering PROM");
 *   ...
 *   while (...) {
 *       exec6502(chunk);
 *       emu_trace_heartbeat(pc, a, x, y, sp, clockticks6502);
 *   }
 *
 * emu_trace_heartbeat() is intentionally cheap to call every loop
 * iteration -- it internally rate-limits itself (only actually prints
 * every EMU_TRACE_HEARTBEAT_INTERVAL calls) so callers don't need to
 * hand-roll their own "print every Nth iteration" counter.
 */
#ifndef EMU_TRACE_H
#define EMU_TRACE_H

#include <stdint.h>

/* How many emu_trace_heartbeat() calls between actual UART writes.
 * Tuned so a typical exec6502(20000)-per-chunk boot loop prints roughly
 * once every 200,000-1,000,000 executed 6502 cycles -- frequent enough
 * to catch a real stall within a couple seconds of wall-clock time,
 * rare enough not to flood the UART and slow down the boot itself. */
#define EMU_TRACE_HEARTBEAT_INTERVAL 10u

/* Byte-sink function pointer, same contract as
 * uart_keyboard_bridge.h's rx functions: caller provides how bytes
 * actually get written (real UART MMIO, a host-native stdout wrapper
 * for test builds, anything). */
typedef void (*emu_trace_putc_fn)(uint8_t byte);

/* Must be called once before any other emu_trace_* function. */
void emu_trace_init(emu_trace_putc_fn putc_fn);

/* Prints a fixed, human-readable text checkpoint marker, e.g.
 * "boot: entering PROM". Use these at meaningful phase transitions
 * (entered boot ROM, disk read started/completed, ramfb registered,
 * cycle budget exhausted) -- cheap, unconditional, always printed
 * (unlike heartbeat, which self-rate-limits). */
void emu_trace_checkpoint(const char *msg);

/* Prints a compact one-line register/cycle snapshot IF this call is the
 * Nth since the last one that actually printed (see
 * EMU_TRACE_HEARTBEAT_INTERVAL) -- cheap enough to call unconditionally
 * every boot-loop iteration. Format:
 *   "hb pc=XXXX a=XX x=XX y=XX sp=XX cyc=XXXXXXXX\n"
 * Comparing pc/cyc across successive heartbeat lines is the actual
 * "is it alive" signal: a genuinely stuck/crashed CPU shows an
 * unchanging (or degenerate few-address-spinning) pc across many
 * heartbeats; real progress shows pc and cyc both advancing. */
void emu_trace_heartbeat(uint16_t pc, uint8_t a, uint8_t x, uint8_t y,
                          uint8_t sp, uint32_t clockticks);

/* Force the next emu_trace_heartbeat() call to actually print,
 * regardless of the interval counter -- useful right after a
 * checkpoint, so the first heartbeat after a phase transition is never
 * silently swallowed by the rate limiter. */
void emu_trace_force_next_heartbeat(void);

#endif /* EMU_TRACE_H */
