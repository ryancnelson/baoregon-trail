#ifndef BOOT_SPLASH_H
#define BOOT_SPLASH_H

#include <stdint.h>
#include "cartridge_layout.h"

/*
 * boot_splash.c -- Retro boot-splash multi-game cartridge selector.
 *
 * Per BRAINSTORM.md section 5: a custom retro boot splash screen lets the
 * 3 physical badge buttons (PREV, NEXT, SELECT) cycle through
 * cartridge_slots[] (cartridge_layout.h) and pick a game, pointing the
 * $C0E0-$C0EF disk-read trap at the selected ReRAM offset via
 * disk_trap_set_image() and resetting the 6502.
 *
 * Button semantics (BRAINSTORM.md section 5's "Logical Mapping"):
 *   Button 0 -- PREV   : move selection to the previous slot (wraps).
 *   Button 1 -- NEXT   : move selection to the next slot (wraps).
 *   Button 2 -- SELECT : confirm the currently highlighted slot -- points
 *                         the disk trap at it and signals a reset is
 *                         needed (caller resets the 6502; this module
 *                         does not touch cpu6502.c directly).
 *
 * Hardware input polling (real GPIO reads) is a follow-up once the Dabao
 * SDK's button/GPIO driver lands -- this module takes an injected
 * "read a button" callback (boot_splash_button_poll_fn) so the selection
 * state machine and disk_trap_set_image() wiring can be RED-GREEN tested
 * on host now, matching the same mock-then-swap-to-real-hardware pattern
 * used throughout this project (e.g. video_apple2.c's read6502_fn,
 * bunnie_audio.c's memory-mapped-flag mechanism).
 *
 * disk_trap_set_image() takes a ReRAM byte pointer (const uint8_t *), but
 * cartridge_slots[] stores an absolute ReRAM address as a uint32_t (per
 * cartridge_layout.h -- matches tools/embed_disk.py's addressing
 * convention). On real XIP hardware that address IS directly
 * dereferenceable ReRAM (see BRAINSTORM.md section 4's
 * disk_image_reram pointer). On host, boot_splash_select_current_game()
 * takes an injected disk-image-setter callback matching
 * disk_trap_set_image()'s exact signature, so tests can capture and
 * assert on the pointer without a real 4 MiB ReRAM-backed address space.
 */

typedef enum {
    BOOT_SPLASH_BUTTON_NONE = 0,
    BOOT_SPLASH_BUTTON_PREV = 1,   /* Button 0 */
    BOOT_SPLASH_BUTTON_NEXT = 2,   /* Button 1 */
    BOOT_SPLASH_BUTTON_SELECT = 3, /* Button 2 */
} boot_splash_button_t;

/* Matches disk_trap_set_image()'s signature exactly (disk_trap.h) -- real
 * callers pass disk_trap_set_image itself; tests inject a mock to capture
 * the pointer without needing real ReRAM-backed memory at that address. */
typedef void (*boot_splash_disk_image_setter_fn)(const uint8_t *image);

typedef struct {
    /* Index into cartridge_slots[], always in [0, CARTRIDGE_SLOT_COUNT). */
    int selected_index;
} boot_splash_state_t;

/* Initialize state with the first cartridge slot highlighted. */
void boot_splash_init(boot_splash_state_t *state);

/*
 * Handle one button press/event. PREV/NEXT move selected_index (wrapping
 * at both ends of cartridge_slots[]); SELECT calls on_select with the
 * currently selected slot's ReRAM address (cast to a byte pointer, per
 * disk_trap_set_image()'s signature) and returns 1 to signal the caller
 * should now reset the 6502 (per BRAINSTORM.md section 5's flow). NONE
 * and any call where button == SELECT with on_select == 0 are no-ops that
 * return 0.
 *
 * Returns 1 if a game was just selected (caller must reset6502()), 0
 * otherwise.
 */
int boot_splash_handle_button(boot_splash_state_t *state, boot_splash_button_t button,
                               boot_splash_disk_image_setter_fn on_select);

/* Convenience accessor: the cartridge_slot_t currently highlighted. */
const cartridge_slot_t *boot_splash_current_slot(const boot_splash_state_t *state);

/*
 * Edge-detection state for boot_splash_poll_apple2_mem_buttons(). Real
 * badge buttons are level-based (apple2_mem_get_button_state() reports
 * "currently held", not "just pressed") -- without edge detection, a
 * button held down for multiple poll calls would fire PREV/NEXT/SELECT
 * repeatedly per call instead of once per physical press. Tracks the
 * previous poll's raw pressed/released state for PB0/PB1/PB2 so only the
 * released->pressed transition (the "edge") triggers a boot_splash event.
 */
typedef struct {
    int was_pressed[3]; /* previous poll's raw state for PB0/PB1/PB2 */
} boot_splash_button_edge_state_t;

/* Initialize edge-detection state assuming all buttons start released
 * (matches real hardware's idle state and apple2_mem_reset()'s default). */
void boot_splash_button_edge_state_init(boot_splash_button_edge_state_t *edge_state);

/*
 * Poll apple2_mem.c's real pushbutton state (apple2_mem_get_button_state(),
 * PB0/PB1/PB2 mapped to PREV/NEXT/SELECT per BRAINSTORM.md section 5's
 * "Logical Mapping") and drive one boot_splash_handle_button() call per
 * newly-pressed button (edge-triggered, not level-triggered -- a button
 * held across multiple polls only fires once, on the release->pressed
 * transition). If more than one button transitions to pressed in the same
 * poll (unlikely on real hardware, but not impossible), PB0 (PREV) is
 * serviced first, matching array index order.
 *
 * Returns 1 if a game was just selected this poll (caller must
 * reset6502()), 0 otherwise -- same contract as boot_splash_handle_button().
 */
int boot_splash_poll_apple2_mem_buttons(boot_splash_state_t *state,
                                         boot_splash_button_edge_state_t *edge_state,
                                         boot_splash_disk_image_setter_fn on_select);

/* Directly select a game slot by index (clamping out-of-bounds indices to valid range)
 * and trigger on_select callback. Returns 1 if game selected (caller must reset6502()),
 * 0 if state or on_select is NULL. */
int boot_splash_select_slot_by_index(boot_splash_state_t *state, int slot_index,
                                       boot_splash_disk_image_setter_fn on_select);

/* Return the currently highlighted slot index (0 to CARTRIDGE_SLOT_COUNT - 1).
 * Safely clamps out-of-bounds indices or NULL state to 0. */
int boot_splash_get_selected_slot_index(const boot_splash_state_t *state);

#endif /* BOOT_SPLASH_H */
