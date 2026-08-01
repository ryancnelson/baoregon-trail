/*
 * boot_splash.c -- Retro boot-splash multi-game cartridge selector.
 * See boot_splash.h for design rationale.
 */
#include "boot_splash.h"
#include "apple2_mem.h"
#include <stddef.h>

void boot_splash_init(boot_splash_state_t *state) {
    if (!state) return;
    state->selected_index = 0;
}

const cartridge_slot_t *boot_splash_current_slot(const boot_splash_state_t *state) {
    if (!state) return NULL;
    int idx = state->selected_index;
    if (idx < 0 || idx >= CARTRIDGE_SLOT_COUNT) {
        idx = 0;
    }
    return &cartridge_slots[idx];
}

int boot_splash_handle_button(boot_splash_state_t *state, boot_splash_button_t button,
                               boot_splash_disk_image_setter_fn on_select) {
    if (!state) return 0;

    switch (button) {
        case BOOT_SPLASH_BUTTON_PREV:
            state->selected_index--;
            if (state->selected_index < 0) {
                state->selected_index = CARTRIDGE_SLOT_COUNT - 1;
            }
            return 0;

        case BOOT_SPLASH_BUTTON_NEXT:
            state->selected_index++;
            if (state->selected_index >= CARTRIDGE_SLOT_COUNT) {
                state->selected_index = 0;
            }
            return 0;

        case BOOT_SPLASH_BUTTON_SELECT:
            if (on_select == 0) {
                return 0;
            }
            {
                const cartridge_slot_t *slot = boot_splash_current_slot(state);
                if (slot) {
                    on_select((const uint8_t *)(uintptr_t)slot->reram_addr);
                }
            }
            return 1;

        case BOOT_SPLASH_BUTTON_NONE:
        default:
            return 0;
    }
}

void boot_splash_button_edge_state_init(boot_splash_button_edge_state_t *edge_state) {
    if (!edge_state) return;
    edge_state->was_pressed[0] = 0;
    edge_state->was_pressed[1] = 0;
    edge_state->was_pressed[2] = 0;
}

/* PB0/PB1/PB2 -> PREV/NEXT/SELECT, matching BRAINSTORM.md section 5's
 * "Logical Mapping" table (Button 0 = PREV, Button 1 = NEXT, Button 2 =
 * SELECT). Kept as a small lookup rather than a switch so adding a 4th
 * button later is a one-line table change. */
static const boot_splash_button_t PB_INDEX_TO_BUTTON[3] = {
    BOOT_SPLASH_BUTTON_PREV,
    BOOT_SPLASH_BUTTON_NEXT,
    BOOT_SPLASH_BUTTON_SELECT,
};

int boot_splash_poll_apple2_mem_buttons(boot_splash_state_t *state,
                                         boot_splash_button_edge_state_t *edge_state,
                                         boot_splash_disk_image_setter_fn on_select) {
    if (!state || !edge_state) return 0;
    int reset_needed = 0;

    for (int pb_index = 0; pb_index < 3; pb_index++) {
        int now_pressed = apple2_mem_get_button_state(pb_index) ? 1 : 0;
        int was_pressed = edge_state->was_pressed[pb_index];

        if (now_pressed && !was_pressed) {
            /* Released->pressed edge: fire exactly once per physical
             * press, not once per poll while held. */
            int this_reset = boot_splash_handle_button(state, PB_INDEX_TO_BUTTON[pb_index], on_select);
            if (this_reset) {
                reset_needed = 1;
            }
        }

        edge_state->was_pressed[pb_index] = now_pressed;
    }

    return reset_needed;
}

int boot_splash_select_slot_by_index(boot_splash_state_t *state, int slot_index,
                                       boot_splash_disk_image_setter_fn on_select) {
    if (!state || !on_select) return 0;

    if (slot_index < 0) {
        slot_index = 0;
    } else if (slot_index >= CARTRIDGE_SLOT_COUNT) {
        slot_index = CARTRIDGE_SLOT_COUNT - 1;
    }

    state->selected_index = slot_index;
    const cartridge_slot_t *slot = boot_splash_current_slot(state);
    if (slot) {
        on_select((const uint8_t *)(uintptr_t)slot->reram_addr);
        return 1;
    }

    return 0;
}
