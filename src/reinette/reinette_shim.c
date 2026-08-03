/*
 * reinette_shim.c -- glue layer connecting reinette_core.c (SDL2-free
 * Apple II hardware core) to this project's own bunnie_audio.c,
 * uart_keyboard_bridge-style UART input, and bio_display/text_apple2
 * rendering pipeline, for the spike-reinette-port branch.
 *
 * Per NEXT_STEPS_REINETTE_SPIKE.md's documented next steps:
 *
 * 1. Speaker: reinette_core.c's $C030/$C020/$C033 softswitch handler
 *    calls a registered void(void) callback (reinette_set_speaker_callback()).
 *    bunnie_audio_trigger_toggle() takes a bunnie_audio_state_t* instead,
 *    so this file owns a static state instance and wraps it in a
 *    zero-arg closure to match reinette's callback signature.
 *
 * 2. Keyboard: reinette's keyboard latch (reinette_KBD) uses the SAME
 *    real Apple II convention as this project's own apple2_mem.c
 *    (bit 7 = strobe pending, low 7 bits = ASCII; $C010 clears bit 7)
 *    but is a completely separate global (reinette_core.c has its own
 *    independent memory model, not apple2_mem.c's). This file reads
 *    from the same injectable UART function-pointer pattern
 *    uart_keyboard_bridge.h established (uart_rx_ready_fn/
 *    uart_rx_read_fn) but writes directly into reinette_KBD instead of
 *    going through apple2_mem_inject_key() (which would write to the
 *    WRONG memory model's latch). LF->CR translation matches
 *    uart_keyboard_bridge.c's existing convention exactly, since real
 *    Apple II software expects CR (0x0D) for Return, not LF.
 *
 * 3. Video: reinette's TEXT-mode screen memory layout ($0400-$07FF
 *    Page 1 / $0800-$0BFF Page 2, same non-linear row-interleave) is
 *    IDENTICAL to this project's own apple2_mem.c layout -- confirmed
 *    by reading reinette_core.c directly (readMem()/writeMem() below
 *    REINETTE_RAMSIZE == reinette_ram[address], no different offset
 *    scheme). This means this project's own text_apple2_render_frame()
 *    (real, MAME-verified 342-0133-a.chr character ROM, already proven
 *    correct against a real DOS 3.3 boot) can be reused DIRECTLY against
 *    reinette_ram via readMem(), with NO NEED to re-embed reinette's own
 *    font-normal.bmp/font-reverse.bmp bitmaps as originally planned in
 *    NEXT_STEPS_REINETTE_SPIKE.md step 2 -- a real simplification found
 *    during this session, not a shortcut around missing work. LoRes/
 *    HiRes reuse this project's lores_apple2.c/video_apple2.c the same
 *    way, via bio_display_render_frame_auto_text_aware() which already
 *    dispatches on TEXT/HIRES/LORES/MIXED exactly as reinette_TEXT/
 *    reinette_HIRES/reinette_PAGE2/reinette_MIXED encode.
 */

#include "reinette_core.h"
#include "reinette_shim.h"
#include "bunnie_audio.h"
#include "bio_display.h"
#include "apple2_mem.h" /* for read6502_fn's underlying uint8_t(*)(uint16_t) shape only -- NOT calling any apple2_mem_* state functions, reinette has its own independent memory */

/* ================================================== SPEAKER (BIO Core 1) */

static bunnie_audio_state_t g_reinette_audio_state;

static void reinette_shim_speaker_toggle(void) {
    bunnie_audio_trigger_toggle(&g_reinette_audio_state);
}

void reinette_shim_audio_init(void) {
    bunnie_audio_init(&g_reinette_audio_state);
    reinette_set_speaker_callback(reinette_shim_speaker_toggle);
}

bunnie_audio_state_t *reinette_shim_get_audio_state(void) {
    return &g_reinette_audio_state;
}

/* ================================================= KEYBOARD (UART bridge) */

int reinette_shim_uart_poll(reinette_uart_rx_ready_fn is_ready, reinette_uart_rx_read_fn read_byte) {
    if (!is_ready || !read_byte) {
        return 0;
    }
    int drained = 0;
    while (is_ready()) {
        uint8_t byte = read_byte();
        if (byte == '\n') {
            byte = '\r'; /* LF -> CR, matches uart_keyboard_bridge.c's convention:
                          * real Apple II software expects CR (0x0D) for Return,
                          * most terminal clients send LF. */
        }
        reinette_KBD = (uint8_t)((byte & 0x7Fu) | 0x80u); /* real Apple II keyboard
                                                            * latch convention: bit 7
                                                            * set = strobe pending,
                                                            * matches reinette_core.c's
                                                            * own $C000/$C010 handling
                                                            * (softSwitches() case
                                                            * 0xC010: reinette_KBD &= 0x7F). */
        drained++;
    }
    return drained;
}

/* ===================================================== VIDEO (BIO Core 0) */

/* readMem() has the exact uint8_t(*)(uint16_t) shape bio_display.h's
 * read6502_fn typedef expects -- pass it straight through, no adapter
 * needed. See file header comment for why this project's own
 * text_apple2_render_frame()/bio_display_render_frame_auto_text_aware()
 * can be reused directly against reinette's memory instead of
 * re-embedding reinette's own font bitmaps. */
void reinette_shim_render_frame(uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]) {
    bio_display_render_frame_auto_text_aware(
        reinette_HIRES ? 1 : 0,
        reinette_PAGE2 ? 1 : 0,
        reinette_MIXED ? 1 : 0,
        reinette_TEXT ? 1 : 0,
        readMem,
        framebuffer);
}
