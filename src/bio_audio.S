/*
 * bio_audio.S -- BIO Core 1 firmware: $C030 speaker PWM toggle.
 *
 * Target: PicoRV32 BIO coprocessor, RV32E (16-register embedded RISC-V,
 * x0-x15 only) per MEMORY.md 2026-07-31's baochip-confirmed correction.
 * Build with -march=rv32e -mabi=ilp32e.
 *
 * Implements the BIO-Core-1 side of the contract already proven in C by
 * src/bunnie_audio.c's bunnie_audio_poll_and_apply() (host-tested,
 * tests/test_bunnie_audio.c) -- this is the hand-written asm port of that
 * SAME logic for the actual BIO core target, not a new design. State
 * layout must match bunnie_audio_state_t exactly:
 *
 *   struct bunnie_audio_state_t {
 *       uint8_t pwm_pin_state;   // offset 0
 *       uint8_t toggle_pending;  // offset 1
 *   };
 *
 * uint8_t bio_audio_poll_and_apply(bunnie_audio_state_t *state):
 *   a0 (x10) = pointer to state on entry, return value on exit (pin state).
 *   Uses only t0 (x5), t1 (x6) as scratch -- all within rv32e's x0-x15
 *   register window, no x16-x31 use anywhere in this file.
 *
 * NOTE ON SCOPE: this function does NOT touch a real PWM hardware pin --
 * no SPI/PWM peripheral register map exists yet (Baochip-1x silicon isn't
 * in hand, see MEMORY.md 2026-07-31 "host-simulator-first" note). The
 * actual GPIO/PWM MMIO write is a follow-up iteration once baochip's
 * hardware-init sequence and peripheral register map are confirmed --
 * cross-domain dependency, not invented solo. This function's job for now
 * is proving the register-window-constrained control flow is correct and
 * matches the already-verified C reference logic bit-for-bit.
 *
 * Verified RED (against an intentionally-wrong "always return 0" stub,
 * QEMU virt exit code 1 = FAIL) before this GREEN implementation (QEMU
 * virt exit code 0 = PASS) -- see tests/test_bio_audio_qemu.S.
 */

    .section .text.bio_audio_poll_and_apply
    .global bio_audio_poll_and_apply
    .type bio_audio_poll_and_apply, @function

bio_audio_poll_and_apply:
    lbu  t0, 1(a0)          /* t0 = toggle_pending */
    beqz t0, .Lno_toggle    /* nothing pending -> skip straight to return */

    lbu  t1, 0(a0)          /* t1 = current pwm_pin_state */
    xori t1, t1, 1          /* flip bit 0 (0<->1, matches C's ternary) */
    sb   t1, 0(a0)          /* store flipped pin state */
    sb   zero, 1(a0)        /* clear toggle_pending */

.Lno_toggle:
    lbu  a0, 0(a0)          /* return (possibly just-updated) pin state */
    ret

    .size bio_audio_poll_and_apply, . - bio_audio_poll_and_apply
