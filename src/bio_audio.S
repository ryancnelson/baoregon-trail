/*
 * bio_audio.S -- BIO Core 1 firmware: $C030 speaker PWM toggle.
 *
 * Target: PicoRV32 BIO coprocessor, RV32IMC (32-register RISC-V with
 * Integer/Multiply/Compressed extensions), x16-x31 reserved for
 * peripheral MMIO via -ffixed-x16..-ffixed-x31 (see BRAINSTORM.md's
 * 2026-08-01 baochip-confirmed correction, which supersedes the earlier
 * RV32E assumption this file was originally written against).
 * Build with -march=rv32imc -mabi=ilp32.
 *
 * This file was NOT rewritten for the ISA correction -- it never needed
 * to be. It only ever used t0 (x5), t1 (x6), a0 (x10), all within the
 * low register window that's valid and unreserved under BOTH the old
 * (incorrect) RV32E model and the corrected RV32IMC model. Verified by
 * rebuilding under -march=rv32imc -mabi=ilp32 and re-running the exact
 * same QEMU RED/GREEN harness (tests/test_bio_audio_qemu.S) unchanged --
 * passes identically. Only the build flags needed to change.
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
 *   Uses only t0 (x5), t1 (x6) as scratch -- well within the low
 *   registers, none of which are reserved MMIO registers under the
 *   corrected RV32IMC model (only x16-x31 are reserved).
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
 * virt exit code 0 = PASS) -- see tests/test_bio_audio_qemu.S. Re-verified
 * RED/GREEN again after the -march=rv32imc migration (2026-08-01).
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
