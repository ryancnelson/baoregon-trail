#ifndef BUNNIE_AUDIO_H
#define BUNNIE_AUDIO_H

#include <stdint.h>

/*
 * BIO Core 1 domain: $C030 speaker toggle -> PWM pin signaling.
 *
 * Per BRAINSTORM.md section 3 + the 3-way trap-mechanism decision locked
 * 2026-07-31 (me/Duke/baochip, see MEMORY.md): the Apple II toggles speaker
 * state on ANY access (read or write) to $C030. apple2_mem.c's read6502()/
 * write6502() trap that address and must call bunnie_audio_trigger_toggle()
 * -- a fire-and-forget signal that must NOT stall the 6502 loop.
 *
 * Mechanism: a memory-mapped flag/register that BIO Core 1 polls
 * (bunnie_audio_poll_and_apply()), NOT a callback and NOT an IPC queue --
 * simplest option for a single-bit toggle, no queue-management overhead.
 *
 * Two sides, two functions:
 *   - Main CPU side (called from the $C030 trap): bunnie_audio_trigger_toggle()
 *     just raises a pending flag and returns immediately. It never touches
 *     the actual PWM pin state.
 *   - BIO Core 1 side (called from its polling loop): bunnie_audio_poll_and_apply()
 *     checks the pending flag; if set, flips the PWM pin level and clears the
 *     flag. This is where the real hardware pin write happens (later, once
 *     ported to bio_core1.c -- for now this returns the pin level for the
 *     host test harness to assert against).
 */
typedef struct {
    volatile uint8_t pwm_pin_state; /* current PWM pin level: 0 or 1 */
    volatile uint8_t toggle_pending; /* set by main CPU, cleared by BIO Core 1 */
    volatile uint32_t toggle_count;  /* total speaker toggle pulses processed */
} bunnie_audio_state_t;

/* Initialize to a known state: pin low, no toggle pending. */
void bunnie_audio_init(bunnie_audio_state_t *state);

/*
 * Main CPU side. Call this from apple2_mem.c's read6502()/write6502() when
 * address == 0xC030. Must be O(1) and non-blocking -- raises toggle_pending
 * only, never flips pwm_pin_state directly.
 */
void bunnie_audio_trigger_toggle(bunnie_audio_state_t *state);

/*
 * BIO Core 1 side. Call this from the BIO Core 1 polling loop. If a toggle
 * is pending, flips pwm_pin_state and clears the pending flag. Returns the
 * (possibly just-updated) pin state either way.
 */
uint8_t bunnie_audio_poll_and_apply(bunnie_audio_state_t *state);

#endif /* BUNNIE_AUDIO_H */
