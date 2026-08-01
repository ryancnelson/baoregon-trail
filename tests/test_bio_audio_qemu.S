/*
 * test_bio_audio_qemu.S -- bare-metal RV32E test harness for
 * bio_audio_poll_and_apply(), run under QEMU's virt machine (no OS, no
 * libc). Uses the SiFive test-finisher MMIO device at 0x100000 to report
 * PASS (0x5555, QEMU exits 0) or FAIL (0x3333, QEMU exits nonzero) --
 * see tools/run_bio_audio_qemu_test.sh for how this is invoked/checked.
 *
 * This is a REAL execution test of the actual rv32e machine code that
 * will run on the PicoRV32 BIO core -- not a host-native mock. Per
 * CLAUDE.md's TDD discipline extended to firmware: RED verified by first
 * running this harness against bio_audio.S's initial always-return-0 stub
 * (FAIL, wrong pin state after a triggered toggle) before the minimal
 * real implementation.
 *
 * Test plan (mirrors tests/test_bunnie_audio.c's already-host-verified
 * cases, ported to the actual target ISA):
 *   1. state = {0, 0}; poll() -> must return 0, state unchanged (no-op
 *      case, mirrors test_poll_is_a_noop_when_no_toggle_pending).
 *   2. state = {0, 1} (toggle pending); poll() -> must return 1, and
 *      state.toggle_pending must now be 0 (mirrors
 *      test_poll_flips_pin_and_clears_pending_when_toggle_was_triggered).
 *   3. state = {1, 1} (pin already high, toggle pending); poll() -> must
 *      return 0 (flips high->low) and toggle_pending cleared.
 *
 * Register budget: only x0-x15 used anywhere in this file (rv32e limit).
 */

    .section .text.entry
    .global _start

FINISHER_ADDR = 0x100000
FINISHER_PASS = 0x5555          /* exit code 0 encoded in the low bits */
FINISHER_FAIL = 0x00013333      /* (exit_code=1 << 16) | FAIL type 0x3333 */

_start:
    /* --- Case 1: no toggle pending -> no-op --- */
    la   a0, state1         /* a0 = &state1 {pwm=0, pending=0} */
    call bio_audio_poll_and_apply
    li   t0, 0
    bne  a0, t0, .Lfail      /* expected return value 0 */
    lbu  t0, state1          /* pwm_pin_state must remain 0 */
    bnez t0, .Lfail
    lbu  t0, state1+1        /* toggle_pending must remain 0 */
    bnez t0, .Lfail

    /* --- Case 2: toggle pending, pin starts low -> flips high --- */
    la   a0, state2          /* a0 = &state2 {pwm=0, pending=1} */
    call bio_audio_poll_and_apply
    li   t0, 1
    bne  a0, t0, .Lfail      /* expected return value 1 */
    lbu  t0, state2          /* pwm_pin_state must now be 1 */
    li   t1, 1
    bne  t0, t1, .Lfail
    lbu  t0, state2+1        /* toggle_pending must now be 0 */
    bnez t0, .Lfail

    /* --- Case 3: toggle pending, pin starts high -> flips low --- */
    la   a0, state3          /* a0 = &state3 {pwm=1, pending=1} */
    call bio_audio_poll_and_apply
    li   t0, 0
    bne  a0, t0, .Lfail      /* expected return value 0 */
    lbu  t0, state3          /* pwm_pin_state must now be 0 */
    bnez t0, .Lfail
    lbu  t0, state3+1        /* toggle_pending must now be 0 */
    bnez t0, .Lfail

.Lpass:
    li   t0, FINISHER_ADDR
    li   t1, FINISHER_PASS
    sw   t1, 0(t0)
    j    .Lspin

.Lfail:
    li   t0, FINISHER_ADDR
    li   t1, FINISHER_FAIL
    sw   t1, 0(t0)

.Lspin:
    j    .Lspin

    .section .data
    .align 2
state1: .byte 0, 0
state2: .byte 0, 1
state3: .byte 1, 1
