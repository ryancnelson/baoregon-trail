/*
 * tests/test_emulator_loop_copy_framebuffer_null_safety.c -- unit test verifying
 * baoregon_emulator_copy_framebuffer() safely rejects NULL destination buffers,
 * zero count, and insufficient buffer capacity (< 320*240) with -1.
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "../src/emulator_loop.h"

int main(void) {
    baoregon_emulator_init();

    uint16_t dummy_buf[320 * 240];

    /* Reject NULL destination buffer */
    assert(baoregon_emulator_copy_framebuffer(NULL, 320 * 240) == -1);

    /* Reject zero count */
    assert(baoregon_emulator_copy_framebuffer(dummy_buf, 0) == -1);

    /* Reject count less than 320*240 */
    assert(baoregon_emulator_copy_framebuffer(dummy_buf, (320 * 240) - 1) == -1);

    /* Accept valid buffer and exact required count */
    assert(baoregon_emulator_copy_framebuffer(dummy_buf, 320 * 240) == 0);

    /* Accept valid buffer and larger count */
    assert(baoregon_emulator_copy_framebuffer(dummy_buf, (320 * 240) + 100) == 0);

    printf("PASS: baoregon_emulator_copy_framebuffer null and bounds safety verified\n");
    printf("All tests passed.\n");
    return 0;
}
