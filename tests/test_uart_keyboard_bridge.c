/*
 * tests/test_uart_keyboard_bridge.c -- TDD tests for the QEMU UART RX ->
 * Apple II keyboard latch bridge (NEXT_STEPS.md Step 9's "Interactive
 * UART Keyboard Softswitches" item).
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../src/uart_keyboard_bridge.h"
#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"

/* Mock UART: a small FIFO the test fills before calling
 * uart_keyboard_bridge_poll(), draining exactly like a real 16550 would
 * (is_ready() reflects "still bytes left", read_byte() consumes one). */
static uint8_t g_mock_fifo[16];
static int g_mock_fifo_head = 0;
static int g_mock_fifo_count = 0;

static void mock_fifo_reset(void) {
    g_mock_fifo_head = 0;
    g_mock_fifo_count = 0;
}

static void mock_fifo_push(uint8_t b) {
    g_mock_fifo[(g_mock_fifo_head + g_mock_fifo_count) % 16] = b;
    g_mock_fifo_count++;
}

static int mock_is_ready(void) {
    return g_mock_fifo_count > 0;
}

static uint8_t mock_read_byte(void) {
    uint8_t b = g_mock_fifo[g_mock_fifo_head];
    g_mock_fifo_head = (g_mock_fifo_head + 1) % 16;
    g_mock_fifo_count--;
    return b;
}

static void test_single_byte_injected_into_keyboard_latch(void) {
    apple2_mem_reset();
    mock_fifo_reset();
    mock_fifo_push('A');

    int drained = uart_keyboard_bridge_poll(mock_is_ready, mock_read_byte);

    assert(drained == 1);
    /* Real Apple II keyboard latch: reading $C000 returns the ASCII
     * value with bit 7 (strobe) set. */
    assert(read6502(0xC000) == ('A' | 0x80));
    printf("PASS: test_single_byte_injected_into_keyboard_latch\n");
}

static void test_drains_all_currently_ready_bytes_in_one_call(void) {
    apple2_mem_reset();
    mock_fifo_reset();
    mock_fifo_push('X');
    mock_fifo_push('Y');
    mock_fifo_push('Z');

    int drained = uart_keyboard_bridge_poll(mock_is_ready, mock_read_byte);

    /* Real single-slot hardware latch: only the LAST byte survives --
     * matches apple2_mem_inject_key()'s own documented overwrite
     * behavior (see test_keyboard_new_key_resets_strobe_even_before_c010
     * in tests/test_apple2_mem.c). This function must NOT try to queue/
     * buffer beyond that one hardware slot. */
    assert(drained == 3);
    assert(read6502(0xC000) == ('Z' | 0x80));
    printf("PASS: test_drains_all_currently_ready_bytes_in_one_call\n");
}

static void test_no_bytes_ready_returns_zero_and_does_not_touch_latch(void) {
    apple2_mem_reset();
    mock_fifo_reset();
    /* Prime the latch with a known prior value first. */
    apple2_mem_inject_key('Q');
    (void)read6502(0xC000); /* consume it, clearing strobe pending state naturally via caller pattern */
    write6502(0xC010, 0x00); /* explicit strobe clear */

    int drained = uart_keyboard_bridge_poll(mock_is_ready, mock_read_byte);

    assert(drained == 0);
    /* Latch value is untouched (still 'Q', strobe bit now clear since
     * nothing new was injected) -- proves polling with nothing ready
     * is a genuine no-op, not an accidental re-inject of stale data. */
    assert((read6502(0xC000) & 0x7F) == 'Q');
    assert((read6502(0xC000) & 0x80) == 0);
    printf("PASS: test_no_bytes_ready_returns_zero_and_does_not_touch_latch\n");
}

static void test_lf_translated_to_cr_for_enter_key(void) {
    /* Real Apple II software expects CR (0x0D) for Return/Enter --
     * Apple II never adopted LF (0x0A) as a line-ending convention, but
     * most modern terminal clients/telnet send LF for Enter. Translate
     * so interactive typing's Enter key actually works, matching
     * poll_uart_input()'s original inline translation in
     * main_qemu_disk2boot.c. */
    apple2_mem_reset();
    mock_fifo_reset();
    mock_fifo_push('\n');

    int drained = uart_keyboard_bridge_poll(mock_is_ready, mock_read_byte);

    assert(drained == 1);
    assert(read6502(0xC000) == ('\r' | 0x80));
    printf("PASS: test_lf_translated_to_cr_for_enter_key\n");
}

static void test_cr_passes_through_unmodified(void) {
    /* A client that already sends real CR directly (0x0D) must not be
     * double-translated or altered. */
    apple2_mem_reset();
    mock_fifo_reset();
    mock_fifo_push('\r');

    int drained = uart_keyboard_bridge_poll(mock_is_ready, mock_read_byte);

    assert(drained == 1);
    assert(read6502(0xC000) == ('\r' | 0x80));
    printf("PASS: test_cr_passes_through_unmodified\n");
}

static void test_null_function_pointers_are_safe_noop(void) {
    apple2_mem_reset();
    mock_fifo_reset();
    mock_fifo_push('A'); /* would be drained if the null-safety check were broken */

    int drained1 = uart_keyboard_bridge_poll(NULL, mock_read_byte);
    int drained2 = uart_keyboard_bridge_poll(mock_is_ready, NULL);
    int drained3 = uart_keyboard_bridge_poll(NULL, NULL);

    assert(drained1 == 0);
    assert(drained2 == 0);
    assert(drained3 == 0);
    /* Latch must be completely untouched -- still default post-reset
     * state (ascii=0, strobe clear), not 'A'. */
    assert(read6502(0xC000) == 0x00);
    printf("PASS: test_null_function_pointers_are_safe_noop\n");
}

int main(void) {
    test_single_byte_injected_into_keyboard_latch();
    test_drains_all_currently_ready_bytes_in_one_call();
    test_no_bytes_ready_returns_zero_and_does_not_touch_latch();
    test_lf_translated_to_cr_for_enter_key();
    test_cr_passes_through_unmodified();
    test_null_function_pointers_are_safe_noop();
    printf("All tests passed.\n");
    return 0;
}
