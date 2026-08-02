#ifndef UART_KEYBOARD_BRIDGE_H
#define UART_KEYBOARD_BRIDGE_H

#include <stdint.h>

/*
 * BIO Core 0 domain: bridges QEMU 16550 UART RX bytes into the Apple II
 * hardware keyboard latch ($C000 read / $C010 strobe-clear), so a real
 * terminal/telnet session attached to QEMU's -serial can type
 * interactively into the emulated Apple II -- NEXT_STEPS.md Step 9's
 * "Interactive UART Keyboard Softswitches" item.
 *
 * The memory-map side of this (apple2_mem_inject_key() setting the
 * ASCII latch + raising the strobe bit, and $C000/$C010's read/strobe-
 * clear dispatch) already existed and is independently tested in
 * tests/test_apple2_mem.c -- that's real Apple II hardware behavior,
 * unrelated to any particular host input source. What was missing is
 * THIS piece: a reusable, host-testable bridge from "bytes arriving on
 * a UART" to "keys injected into the latch", extracted out of what was
 * previously inline, untested, copy-pasted logic living directly inside
 * src/main_qemu_disk2boot.c's poll_uart_input() (duplicated again for
 * every new QEMU demo entry point that wants interactive typing).
 *
 * Real 16550 UART LSR (Line Status Register) semantics: bit 0 (Data
 * Ready) is set when a received byte is waiting in RBR (Receive Buffer
 * Register). Reading RBR consumes that byte and (on real hardware)
 * clears bit 0 until the next byte arrives -- this module doesn't
 * assume that side effect happens automatically, it re-checks
 * is_rx_ready() in a loop so it drains ALL currently-buffered bytes in
 * one call (matches poll_uart_input()'s original `while (*lsr & 0x01)`
 * shape, not just a single byte per call).
 *
 * Deliberately NOT hardcoding the real UART0_BASE MMIO addresses here --
 * takes the ready-check and byte-read as injectable function pointers
 * (same pattern as read6502_fn in video_apple2.h) so this can be fully
 * unit-tested on host with a mock UART, and the real QEMU entry points
 * just pass thin wrappers around the actual volatile MMIO reads.
 */

/* Returns nonzero if a byte is available to read (real hardware: LSR
 * bit 0 set), zero otherwise. */
typedef int (*uart_rx_ready_fn)(void);

/* Reads and consumes one waiting byte. Only ever called when
 * uart_rx_ready_fn() has just returned nonzero -- undefined what a real
 * UART returns otherwise, so this bridge never calls it speculatively. */
typedef uint8_t (*uart_rx_read_fn)(void);

/*
 * Drains all currently-ready bytes from the UART (via is_ready/read_byte)
 * and injects each one into the Apple II keyboard latch via
 * apple2_mem_inject_key(), translating '\n' (LF, what most terminal
 * clients send for Enter) to '\r' (CR, 0x0D -- the actual byte real
 * Apple II software expects for Return/Enter; Apple II never used LF as
 * a line-ending convention). All other bytes pass through unmodified
 * (apple2_mem_inject_key() itself masks to 7 bits).
 *
 * Real Apple II hardware only has ONE keyboard latch slot -- if bytes
 * arrive faster than 6502 code drains $C000/$C010 between calls to this
 * function, earlier undrained keys are silently overwritten by later
 * ones (exactly matching apple2_mem_inject_key()'s own documented
 * overwrite-before-ack behavior, see tests/test_apple2_mem.c's
 * test_keyboard_new_key_resets_strobe_even_before_c010). This function
 * does not queue/buffer keys beyond that single hardware latch slot --
 * that's a deliberate real-hardware-accurate limitation, not a bug.
 *
 * Returns the number of bytes drained (0 if none were ready), so
 * callers/tests can distinguish "polled, nothing arrived" from "no UART
 * present at all" without needing a separate NULL-safety check for that
 * case.
 *
 * Safety: NULL is_ready or read_byte is a safe no-op, returns 0.
 */
int uart_keyboard_bridge_poll(uart_rx_ready_fn is_ready, uart_rx_read_fn read_byte);

#endif /* UART_KEYBOARD_BRIDGE_H */
