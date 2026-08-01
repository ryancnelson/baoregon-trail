/*
 * ramfb_display.h -- QEMU 'ramfb' sysbus display driver for RISC-V virt target.
 */
#ifndef RAMFB_DISPLAY_H
#define RAMFB_DISPLAY_H

#include <stdint.h>
#include <stddef.h>

#define RAMFB_WIDTH  280u
#define RAMFB_HEIGHT 192u
#define RAMFB_STRIDE (RAMFB_WIDTH * 4u) /* XRGB8888 = 4 bytes/pixel */

/* Initialize QEMU fw_cfg and register ramfb device. Returns 0 on success, -1 if ramfb not found. */
int ramfb_init(void);

/* Convert RGB565 buffer (280x192 uint16_t) to XRGB8888 ramfb buffer (280x192 uint32_t) and flush. */
void ramfb_present_frame565(const uint16_t *rgb565_buffer);

/* Read-only access to internal 32-bit XRGB8888 buffer for host unit tests. */
const uint32_t *ramfb_get_buffer(void);

/* Convert single RGB565 pixel to XRGB8888 uint32_t. */
uint32_t ramfb_pixel_565_to_xrgb8888(uint16_t rgb565);

#endif /* RAMFB_DISPLAY_H */
