#include "bunnie_audio.h"

void bunnie_audio_init(bunnie_audio_state_t *state) {
    state->pwm_pin_state = 0;
    state->toggle_pending = 0;
}

void bunnie_audio_trigger_toggle(bunnie_audio_state_t *state) {
    state->toggle_pending = 1;
}

uint8_t bunnie_audio_poll_and_apply(bunnie_audio_state_t *state) {
    if (state->toggle_pending) {
        state->pwm_pin_state = state->pwm_pin_state ? 0 : 1;
        state->toggle_pending = 0;
    }
    return state->pwm_pin_state;
}
