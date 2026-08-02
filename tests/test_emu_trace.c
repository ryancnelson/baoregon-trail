/*
 * test_emu_trace.c -- host-native tests for src/emu_trace.c, the
 * first-class observability module every future demo/boot entry point
 * (QEMU today, real Bunnie-designed Baochip-1x hardware with its own
 * real UART next week) should use for "is my code doing anything"
 * visibility.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/emu_trace.h"

#define CAPTURE_BUF_SIZE 4096
static char g_capture[CAPTURE_BUF_SIZE];
static int g_capture_len = 0;

static void capture_putc(uint8_t byte) {
    if (g_capture_len < CAPTURE_BUF_SIZE - 1) {
        g_capture[g_capture_len++] = (char)byte;
    }
}

static void reset_capture(void) {
    g_capture_len = 0;
    g_capture[0] = 0;
}

static void test_checkpoint_prints_message_and_newline(void) {
    reset_capture();
    emu_trace_init(capture_putc);
    emu_trace_checkpoint("boot: entering PROM");
    g_capture[g_capture_len] = 0;
    assert(strcmp(g_capture, "boot: entering PROM\n") == 0);
    printf("PASS: test_checkpoint_prints_message_and_newline\n");
}

static void test_heartbeat_rate_limits_by_default(void) {
    reset_capture();
    emu_trace_init(capture_putc);
    /* First EMU_TRACE_HEARTBEAT_INTERVAL - 1 calls must print nothing. */
    for (unsigned int i = 0; i < EMU_TRACE_HEARTBEAT_INTERVAL - 1; i++) {
        emu_trace_heartbeat(0x1234, 0, 0, 0, 0, 0);
    }
    assert(g_capture_len == 0);
    printf("PASS: test_heartbeat_rate_limits_by_default\n");
}

static void test_heartbeat_prints_on_nth_call(void) {
    reset_capture();
    emu_trace_init(capture_putc);
    for (unsigned int i = 0; i < EMU_TRACE_HEARTBEAT_INTERVAL; i++) {
        emu_trace_heartbeat(0x1234, 0xAB, 0xCD, 0xEF, 0x01, 0xDEADBEEFu);
    }
    g_capture[g_capture_len] = 0;
    assert(g_capture_len > 0);
    assert(strstr(g_capture, "pc=1234") != NULL);
    assert(strstr(g_capture, "a=AB") != NULL);
    assert(strstr(g_capture, "x=CD") != NULL);
    assert(strstr(g_capture, "y=EF") != NULL);
    assert(strstr(g_capture, "sp=01") != NULL);
    assert(strstr(g_capture, "cyc=DEADBEEF") != NULL);
    printf("PASS: test_heartbeat_prints_on_nth_call\n");
}

static void test_heartbeat_counter_resets_after_printing(void) {
    reset_capture();
    emu_trace_init(capture_putc);
    for (unsigned int i = 0; i < EMU_TRACE_HEARTBEAT_INTERVAL; i++) {
        emu_trace_heartbeat(0, 0, 0, 0, 0, 0);
    }
    int len_after_first_print = g_capture_len;
    assert(len_after_first_print > 0);

    /* One more call right after a print must NOT print again
     * immediately -- counter should have reset to 0, not stayed high. */
    emu_trace_heartbeat(0, 0, 0, 0, 0, 0);
    assert(g_capture_len == len_after_first_print);
    printf("PASS: test_heartbeat_counter_resets_after_printing\n");
}

static void test_force_next_heartbeat_bypasses_rate_limit(void) {
    reset_capture();
    emu_trace_init(capture_putc);
    emu_trace_force_next_heartbeat();
    emu_trace_heartbeat(0x0042, 0, 0, 0, 0, 0);
    g_capture[g_capture_len] = 0;
    assert(g_capture_len > 0);
    assert(strstr(g_capture, "pc=0042") != NULL);
    printf("PASS: test_force_next_heartbeat_bypasses_rate_limit\n");
}

static void test_never_initialized_is_safe_noop(void) {
    /* A fresh process image where emu_trace_init() was never called
     * (e.g. a caller forgets to wire it up) must not crash -- both
     * functions should just silently no-op. This test relies on
     * process-level global state being fresh; since other tests in
     * this same binary DO call emu_trace_init(), we can't literally
     * test "never initialized" mid-binary. Instead this documents the
     * contract via a symbolic re-check: passing a NULL putc_fn
     * explicitly must behave identically to never-initialized. */
    emu_trace_init((emu_trace_putc_fn)0);
    emu_trace_checkpoint("should not crash, should not print");
    emu_trace_heartbeat(1, 2, 3, 4, 5, 6);
    printf("PASS: test_never_initialized_is_safe_noop\n");
}

int main(void) {
    test_checkpoint_prints_message_and_newline();
    test_heartbeat_rate_limits_by_default();
    test_heartbeat_prints_on_nth_call();
    test_heartbeat_counter_resets_after_printing();
    test_force_next_heartbeat_bypasses_rate_limit();
    test_never_initialized_is_safe_noop();
    printf("All tests passed.\n");
    return 0;
}
