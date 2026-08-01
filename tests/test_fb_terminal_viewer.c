/*
 * test_fb_terminal_viewer.c -- verify fb_terminal_viewer_print() produces
 * structurally correct ANSI truecolor output for known RGB565 values.
 *
 * RED test (vertical tracer bullet): prove the ANSI escape codes encode
 * the right 8-bit RGB values for known 5/6/5 inputs (black, white, and
 * one asymmetric color to catch a channel-swap bug), and that the output
 * has exactly BIO_DISPLAY_HEIGHT/2 terminal rows (half-block packing).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FB_TERMINAL_VIEWER_NO_MAIN
#include "../tools/fb_terminal_viewer.c"

/* Redirect stdout to a memory buffer for inspection, since
 * fb_terminal_viewer_print() writes directly to stdout via printf(). */
static char g_captured[1 << 20];

static void capture_call(const uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]) {
    char tmp_path[] = "/tmp/fb_viewer_test_XXXXXX";
    int fd = mkstemp(tmp_path);
    if (fd < 0) {
        fprintf(stderr, "FAIL: mkstemp() failed\n");
        exit(1);
    }
    close(fd);

    /* Duplicate the real stdout fd so it can be restored afterwards --
     * portable and doesn't depend on /dev/tty existing (this test may
     * run in a non-interactive/sandboxed session with no controlling
     * terminal). */
    int saved_stdout_fd = dup(fileno(stdout));
    if (saved_stdout_fd < 0) {
        fprintf(stderr, "FAIL: dup(stdout) failed\n");
        exit(1);
    }
    fflush(stdout);

    FILE *redirected = freopen(tmp_path, "w", stdout);
    if (!redirected) {
        fprintf(stderr, "FAIL: freopen() failed\n");
        exit(1);
    }

    fb_terminal_viewer_print(framebuffer);
    fflush(stdout);

    /* Restore the real stdout fd, then re-point the stdout FILE* at it. */
    dup2(saved_stdout_fd, fileno(stdout));
    close(saved_stdout_fd);
    /* Clear EOF/error flags left over from the redirected stream. */
    clearerr(stdout);

    FILE *readback = fopen(tmp_path, "r");
    size_t n = fread(g_captured, 1, sizeof(g_captured) - 1, readback);
    g_captured[n] = '\0';
    fclose(readback);
    remove(tmp_path);
}

static int count_occurrences(const char *haystack, const char *needle) {
    int count = 0;
    const char *p = haystack;
    size_t needle_len = strlen(needle);
    while ((p = strstr(p, needle)) != NULL) {
        count++;
        p += needle_len;
    }
    return count;
}

static int test_output_has_correct_row_count(void) {
    static uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0, sizeof(framebuffer));

    capture_call(framebuffer);

    /* Each terminal row ends with the ANSI reset "\x1b[0m\n" -- count
     * those to get the row count, since half-block packing means
     * BIO_DISPLAY_HEIGHT/2 terminal rows for BIO_DISPLAY_HEIGHT
     * scanlines. */
    int row_count = count_occurrences(g_captured, "\x1b[0m\n");
    int expected_rows = BIO_DISPLAY_HEIGHT / 2;
    if (row_count != expected_rows) {
        fprintf(stderr, "FAIL: got %d terminal rows, expected %d (half-block packing)\n",
                row_count, expected_rows);
        return 1;
    }
    printf("PASS: test_output_has_correct_row_count\n");
    return 0;
}

static int test_black_pixel_produces_zero_rgb_ansi_code(void) {
    static uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    memset(framebuffer, 0, sizeof(framebuffer)); /* all pixels black (0x0000) */

    capture_call(framebuffer);

    /* Black (0,0,0) foreground: "\x1b[38;2;0;0;0m" */
    if (strstr(g_captured, "\x1b[38;2;0;0;0m") == NULL) {
        fprintf(stderr, "FAIL: expected black foreground ANSI code not found in output\n");
        return 1;
    }
    printf("PASS: test_black_pixel_produces_zero_rgb_ansi_code\n");
    return 0;
}

static int test_white_pixel_produces_max_rgb_ansi_code(void) {
    static uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    for (int i = 0; i < BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT; i++) {
        framebuffer[i] = 0xFFFF; /* all pixels white */
    }

    capture_call(framebuffer);

    /* White (255,255,255): 5-bit 0x1F -> 255, 6-bit 0x3F -> 255. */
    if (strstr(g_captured, "\x1b[38;2;255;255;255m") == NULL) {
        fprintf(stderr, "FAIL: expected white foreground ANSI code not found in output\n");
        return 1;
    }
    printf("PASS: test_white_pixel_produces_max_rgb_ansi_code\n");
    return 0;
}

static int test_pure_green_channel_isolated_correctly(void) {
    /* Catches a channel-order bug (R/G/B swap): 0x07E0 is pure green
     * (RRRRR=0, GGGGGG=max, BBBBB=0) in RGB565 -- must decode to
     * (0, 255, 0), NOT (255,0,0) or (0,0,255). */
    static uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT];
    for (int i = 0; i < BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT; i++) {
        framebuffer[i] = 0x07E0;
    }

    capture_call(framebuffer);

    if (strstr(g_captured, "\x1b[38;2;0;255;0m") == NULL) {
        fprintf(stderr, "FAIL: expected pure-green foreground ANSI code (0,255,0) not found "
                        "-- possible RGB565 channel-order bug\n");
        return 1;
    }
    printf("PASS: test_pure_green_channel_isolated_correctly\n");
    return 0;
}

int main(void) {
    int failures = 0;
    failures += test_output_has_correct_row_count();
    failures += test_black_pixel_produces_zero_rgb_ansi_code();
    failures += test_white_pixel_produces_max_rgb_ansi_code();
    failures += test_pure_green_channel_isolated_correctly();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
