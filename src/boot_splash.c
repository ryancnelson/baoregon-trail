/*
 * boot_splash.c -- Retro boot-splash multi-game cartridge selector.
 * See boot_splash.h for design rationale.
 */
#include "boot_splash.h"

void boot_splash_init(boot_splash_state_t *state) {
    state->selected_index = 0;
}

const cartridge_slot_t *boot_splash_current_slot(const boot_splash_state_t *state) {
    return &cartridge_slots[state->selected_index];
}

int boot_splash_handle_button(boot_splash_state_t *state, boot_splash_button_t button,
                               boot_splash_disk_image_setter_fn on_select) {
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
                on_select((const uint8_t *)(uintptr_t)slot->reram_addr);
            }
            return 1;

        case BOOT_SPLASH_BUTTON_NONE:
        default:
            return 0;
    }
}
