/* RED phase stub -- deliberately wrong (always returns 0 without checking
 * toggle_pending or flipping pwm_pin_state) so the test harness fails for
 * the right reason before the real implementation is written. */
    .section .text.bio_audio_poll_and_apply
    .global bio_audio_poll_and_apply
    .type bio_audio_poll_and_apply, @function

bio_audio_poll_and_apply:
    li a0, 0
    ret

    .size bio_audio_poll_and_apply, . - bio_audio_poll_and_apply
