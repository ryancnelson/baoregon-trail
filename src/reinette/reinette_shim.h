#ifndef REINETTE_SHIM_H
#define REINETTE_SHIM_H

#include <stdint.h>
#include "bunnie_audio.h"
#include "bio_display.h"

/* Wires reinette_core.c's speaker callback to bunnie_audio_trigger_toggle().
 * Call once at boot, before the first exec6502-style CPU run. */
void reinette_shim_audio_init(void);

/* Returns the shim's owned bunnie_audio_state_t, for BIO Core 1's
 * polling loop (bunnie_audio_poll_and_apply()) to consume. */
bunnie_audio_state_t *reinette_shim_get_audio_state(void);

typedef int (*reinette_uart_rx_ready_fn)(void);
typedef uint8_t (*reinette_uart_rx_read_fn)(void);

/* Drains all currently-ready UART bytes and injects each into
 * reinette's own keyboard latch (reinette_KBD) -- NOT
 * apple2_mem_inject_key(), which targets a different, unrelated
 * memory model. Returns the number of bytes drained. Safety: NULL
 * is_ready or read_byte is a safe no-op, returns 0. */
int reinette_shim_uart_poll(reinette_uart_rx_ready_fn is_ready, reinette_uart_rx_read_fn read_byte);

/* Renders reinette's current TEXT/LoRes/HiRes/Mixed video state into a
 * BIO_DISPLAY_WIDTH x BIO_DISPLAY_HEIGHT (280x192) RGB565 framebuffer,
 * reusing this project's own bio_display/text_apple2 renderers directly
 * against reinette_ram (via readMem()) -- see reinette_shim.c's file
 * header comment for why this works without re-embedding reinette's
 * own font bitmaps. */
void reinette_shim_render_frame(uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]);

#endif /* REINETTE_SHIM_H */
