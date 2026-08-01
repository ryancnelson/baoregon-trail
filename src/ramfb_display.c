/*
 * ramfb_display.c -- QEMU 'ramfb' sysbus display driver implementation.
 */
#include "ramfb_display.h"

/* Static framebuffer storage for XRGB8888 format (280x192 uint32_t pixels) */
static uint32_t g_ramfb_buffer[RAMFB_WIDTH * RAMFB_HEIGHT];
static int g_ramfb_initialized = 0;

/* QEMU virt RISC-V fw_cfg registers base address */
#define FW_CFG_BASE 0x10100000u
#define FW_CFG_DATA (*(volatile uint32_t *)(FW_CFG_BASE + 0x00u))
#define FW_CFG_SELECTOR (*(volatile uint16_t *)(FW_CFG_BASE + 0x08u))
#define FW_CFG_DMA (*(volatile uint64_t *)(FW_CFG_BASE + 0x10u))

/* DRM FourCC format for XRGB8888: 'X' | ('R'<<8) | ('G'<<16) | ('B'<<24) = 0x34325258 */
#define DRM_FORMAT_XRGB8888 0x34325258u

/* Big-endian conversion helpers */
static inline uint16_t bswap16(uint16_t x) {
    return (uint16_t)((x << 8) | (x >> 8));
}

static inline uint32_t bswap32(uint32_t x) {
    return ((x & 0x000000FFu) << 24) |
           ((x & 0x0000FF00u) << 8)  |
           ((x & 0x00FF0000u) >> 8)  |
           ((x & 0xFF000000u) >> 24);
}

static inline uint64_t bswap64(uint64_t x) {
    return ((uint64_t)bswap32((uint32_t)x) << 32) | bswap32((uint32_t)(x >> 32));
}

typedef struct __attribute__((packed)) {
    uint64_t addr;   /* Framebuffer physical address (big endian uint64_t) */
    uint32_t fourcc; /* DRM_FORMAT_* FourCC (big endian uint32_t) */
    uint32_t flags;  /* 0 */
    uint32_t width;  /* Framebuffer width in pixels (big endian uint32_t) */
    uint32_t height; /* Framebuffer height in pixels (big endian uint32_t) */
    uint32_t stride; /* Bytes per line = width * 4 (big endian uint32_t) */
} ramfb_cfg_t;

uint32_t ramfb_pixel_565_to_xrgb8888(uint16_t rgb565) {
    uint8_t r5 = (uint8_t)((rgb565 >> 11) & 0x1F);
    uint8_t g6 = (uint8_t)((rgb565 >> 5) & 0x3F);
    uint8_t b5 = (uint8_t)(rgb565 & 0x1F);

    uint8_t r8 = (uint8_t)((r5 << 3) | (r5 >> 2));
    uint8_t g8 = (uint8_t)((g6 << 2) | (g6 >> 4));
    uint8_t b8 = (uint8_t)((b5 << 3) | (b5 >> 2));

    return (0xFFu << 24) | ((uint32_t)r8 << 16) | ((uint32_t)g8 << 8) | (uint32_t)b8;
}

const uint32_t *ramfb_get_buffer(void) {
    return g_ramfb_buffer;
}

int ramfb_init(void) {
    /* Initialize buffer to black */
    for (size_t i = 0; i < (RAMFB_WIDTH * RAMFB_HEIGHT); i++) {
        g_ramfb_buffer[i] = 0xFF000000u;
    }
    g_ramfb_initialized = 1;
    return 0;
}

void ramfb_present_frame565(const uint16_t *rgb565_buffer) {
    if (!rgb565_buffer) return;

    for (size_t i = 0; i < (RAMFB_WIDTH * RAMFB_HEIGHT); i++) {
        g_ramfb_buffer[i] = ramfb_pixel_565_to_xrgb8888(rgb565_buffer[i]);
    }
}
