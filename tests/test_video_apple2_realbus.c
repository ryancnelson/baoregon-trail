#include <stdio.h>

#include "../src/video_apple2.h"
#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"

/*
 * RED test (real-bus integration): prove hires_decode_scanline_mono() and
 * hires_decode_scanline_color() work against apple2_mem.c's REAL
 * read6502()/write6502() -- not the mock used in
 * tests/test_video_apple2.c / test_video_apple2_color.c /
 * test_video_apple2_fullframe.c. Per Ryan's steer 2026-07-31: mock now,
 * swap to the real implementation once it lands -- apple2_mem.c landed in
 * commit 8df86cc, this is the swap.
 *
 * Writes go through write6502() (the same path the emulated 6502 CPU would
 * use to update the Hi-Res buffer), reads go through the real read6502
 * function pointer -- exercising the full path: 6502-visible address space
 * -> apple2_mem.c's plain-RAM backing for $2000-$3FFF -> my un-swizzle
 * logic, with no mock in between.
 */

static int test_mono_decode_reads_byte_written_via_real_write6502(void) {
    apple2_mem_reset();

    /* Row 0 lives at $2000 (offset 0x0000) -- write byte 0x55 there via the
     * real bus write path, exactly as the 6502 core would when Applesoft/
     * DOS pokes the Hi-Res screen. */
    write6502(0x2000, 0x55); /* 0b0101_0101 -> bits 0..6 = 1,0,1,0,1,0,1 */

    uint8_t out_pixels[HIRES_PIXELS_WIDE];
    hires_decode_scanline_mono(0, read6502, out_pixels);

    const uint8_t expected_first7[7] = {1, 0, 1, 0, 1, 0, 1};
    for (int i = 0; i < 7; i++) {
        if (out_pixels[i] != expected_first7[i]) {
            fprintf(stderr, "FAIL: pixel[%d] = %u, expected %u\n",
                    i, out_pixels[i], expected_first7[i]);
            return 1;
        }
    }
    printf("PASS: test_mono_decode_reads_byte_written_via_real_write6502\n");
    return 0;
}

static int test_mono_decode_reads_row8_from_correct_real_bus_offset(void) {
    apple2_mem_reset();

    /* Row 8 lives at $2080 (offset 0x0080) -- writing at the WRONG address
     * ($2000, row 0's slot) must NOT show up when decoding row 8. This
     * proves apple2_mem.c's plain-RAM backing for $2000-$3FFF is addressed
     * the same way my row-offset table expects (byte-for-byte, no
     * remapping/aliasing introduced by the real memory map). */
    write6502(0x2080, 0x2A); /* 0b0010_1010 -> bits 0..6 = 0,1,0,1,0,1,0 */

    uint8_t out_pixels[HIRES_PIXELS_WIDE];
    hires_decode_scanline_mono(8, read6502, out_pixels);

    const uint8_t expected_first7[7] = {0, 1, 0, 1, 0, 1, 0};
    for (int i = 0; i < 7; i++) {
        if (out_pixels[i] != expected_first7[i]) {
            fprintf(stderr, "FAIL: row8 pixel[%d] = %u, expected %u\n",
                    i, out_pixels[i], expected_first7[i]);
            return 1;
        }
    }
    printf("PASS: test_mono_decode_reads_row8_from_correct_real_bus_offset\n");
    return 0;
}

static int test_color_decode_works_against_real_bus(void) {
    apple2_mem_reset();

    /* Isolated lit pixel, high bit set, even column -> ORANGE (per the
     * artifacting rule in video_apple2.h), written through the real bus. */
    write6502(0x2000, 0x81); /* high bit + bit0 (col 0, even) */

    hires_color_t out_colors[HIRES_PIXELS_WIDE];
    hires_decode_scanline_color(0, read6502, out_colors);

    if (out_colors[0] != HIRES_COLOR_ORANGE) {
        fprintf(stderr, "FAIL: color[0] = %d, expected HIRES_COLOR_ORANGE (%d)\n",
                out_colors[0], HIRES_COLOR_ORANGE);
        return 1;
    }
    printf("PASS: test_color_decode_works_against_real_bus\n");
    return 0;
}

static int test_writes_to_soft_switch_region_do_not_leak_into_hires_buffer(void) {
    apple2_mem_reset();

    /* $C030 is the speaker soft-switch, NOT part of the Hi-Res buffer.
     * Triggering it must not corrupt row 0's decode -- proves the real
     * bus's address dispatch (RAM vs soft-switch) doesn't alias into the
     * $2000-$3FFF region my un-swizzler reads from. */
    write6502(0x2000, 0x7F); /* all 7 low bits set: pixels 0-6 all on */
    (void)read6502(0xC030);  /* speaker toggle trap -- must not touch $2000 */

    uint8_t out_pixels[HIRES_PIXELS_WIDE];
    hires_decode_scanline_mono(0, read6502, out_pixels);

    for (int i = 0; i < 7; i++) {
        if (out_pixels[i] != 1) {
            fprintf(stderr,
                    "FAIL: pixel[%d] = %u, expected 1 (soft-switch access "
                    "corrupted the Hi-Res buffer read)\n",
                    i, out_pixels[i]);
            return 1;
        }
    }
    printf("PASS: test_writes_to_soft_switch_region_do_not_leak_into_hires_buffer\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_mono_decode_reads_byte_written_via_real_write6502();
    failures += test_mono_decode_reads_row8_from_correct_real_bus_offset();
    failures += test_color_decode_works_against_real_bus();
    failures += test_writes_to_soft_switch_region_do_not_leak_into_hires_buffer();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
