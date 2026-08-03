/*
 * tests/test_disk2_controller_seek_overshoot_readable.c -- RED test
 * reproducing the real disk-swap RWTS-hang bug found and root-caused
 * this session (see NEXT_STEPS.md's "DUKE'S DISK-SWAP RESEARCH" and
 * "DUKE'S RWTS TRACE" sections): after a seek overshoots past the
 * physical track limit (e.g. DOS 3.3's RWTS issuing more phase-step
 * pulses than the drive has tracks, entirely normal/expected real
 * hardware behavior per Beneath Apple DOS's SEEKABS documentation --
 * real drives have a mechanical hard stop, not an error), a real
 * disk2_controller_access() bus-level address-field read at the
 * clamped position must still succeed and correctly report the REAL
 * clamped track number -- this is what lets real DOS 3.3's RDRIGHT/
 * SETTRK self-correction mechanism trigger and recover.
 *
 * This test proves the read pipeline itself (nibble_shift() +
 * disk2_controller_access()) works fine at the clamp boundary -- which
 * was independently confirmed via a live boot trace this session.
 * Ported from tests/test_disk2_controller_nibble_roundtrip.c's proven
 * bus-level decode helpers (kept duplicated per that file's own stated
 * rationale: an independent read-path check, not sharing code with the
 * encoder under test).
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../src/disk2_controller.h"
#include "../src/cpu6502.h"

static uint8_t g_detrans62[256];
static void init_detrans62(void) {
    static const uint8_t trans62[64] = {
        0x96, 0x97, 0x9A, 0x9B, 0x9D, 0x9E, 0x9F, 0xA6,
        0xA7, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB2, 0xB3,
        0xB4, 0xB5, 0xB6, 0xB7, 0xB9, 0xBA, 0xBB, 0xBC,
        0xBD, 0xBE, 0xBF, 0xCB, 0xCD, 0xCE, 0xCF, 0xD3,
        0xD6, 0xD7, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE,
        0xDF, 0xE5, 0xE6, 0xE7, 0xE9, 0xEA, 0xEB, 0xEC,
        0xED, 0xEE, 0xEF, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6,
        0xF7, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
    };
    memset(g_detrans62, 0xFF, sizeof(g_detrans62));
    for (int i = 0; i < 64; i++) {
        g_detrans62[trans62[i]] = (uint8_t)i;
    }
}

#define LOC_DRIVEON 0x9
#define LOC_DRIVEREAD 0xC
#define LOC_DRIVEREADMODE 0xE
#define LOC_PHASE0ON 0x1
#define LOC_PHASE1ON 0x3
#define LOC_PHASE2ON 0x5
#define LOC_PHASE3ON 0x7

static uint8_t read_one_nibble(disk2_controller_t *ctl) {
    uint8_t b;
    do {
        b = disk2_controller_access(ctl, LOC_DRIVEREAD, 0, 0);
        if ((b & 0x80) == 0) {
            clockticks6502 += 32;
        }
    } while ((b & 0x80) == 0);
    return b;
}

static void seek_to_prolog(disk2_controller_t *ctl, uint8_t p0, uint8_t p1, uint8_t p2) {
    uint8_t window[3] = {0, 0, 0};
    for (;;) {
        uint8_t b = read_one_nibble(ctl);
        window[0] = window[1];
        window[1] = window[2];
        window[2] = b;
        if (window[0] == p0 && window[1] == p1 && window[2] == p2) {
            return;
        }
    }
}

static uint8_t decode_44(disk2_controller_t *ctl) {
    uint8_t xx = read_one_nibble(ctl);
    uint8_t yy = read_one_nibble(ctl);
    return (uint8_t)(((xx << 1) | 0x01) & yy);
}

int main(void) {
    init_detrans62();

    extern void encode_sector_16_for_test(uint8_t volume, uint8_t track, uint8_t sector,
                                           const uint8_t data[256], uint8_t *out, int *out_len);

    static uint8_t sector_data[256];
    for (int i = 0; i < 256; i++) {
        sector_data[i] = (uint8_t)((i * 13 + 7) & 0xFF);
    }

    /* Build the LAST valid track (track 34, DISK2_MAX_TRACKS-1) --
     * the real physical clamp boundary an overshooting seek lands on. */
    static uint8_t track_nibbles[8192];
    int track_len = 0;
    encode_sector_16_for_test(254, DISK2_MAX_TRACKS - 1, 0, sector_data, track_nibbles, &track_len);

    disk2_controller_t ctl;
    disk2_controller_reset(&ctl);

    static disk2_nibble_track_t tracks[DISK2_MAX_TRACKS];
    memset(tracks, 0, sizeof(tracks));
    memcpy(tracks[DISK2_MAX_TRACKS - 1].data, track_nibbles, (size_t)track_len);
    tracks[DISK2_MAX_TRACKS - 1].length = track_len;
    disk2_controller_load_nibble_disk(&ctl, 0, tracks, 0);

    disk2_controller_access(&ctl, LOC_DRIVEON, 1, 0);
    disk2_controller_access(&ctl, LOC_DRIVEREADMODE, 1, 0);

    /* Simulate real RWTS seek OVERSHOOT: issue MANY more phase-step
     * pulses than tracks exist on the disk -- entirely realistic
     * behavior (real RWTS seeks with an intentional safety margin to
     * guarantee reaching track 0/max even from an unknown starting
     * position, relying on the mechanical hard stop plus its own
     * SETTRK self-correction on the next read to recover). This drives
     * d->track well past DISK2_MAX_TRACKS*4 quarter-tracks, requiring
     * the clamp in set_phase() to engage repeatedly. */
    int phase = 0;
    for (int step = 0; step < 200; step++) {
        int next_phase = (phase + 1) % 4;
        int on_offset = (next_phase == 0) ? LOC_PHASE0ON
                       : (next_phase == 1) ? LOC_PHASE1ON
                       : (next_phase == 2) ? LOC_PHASE2ON
                       : LOC_PHASE3ON;
        disk2_controller_access(&ctl, (uint8_t)on_offset, 1, 0);
        phase = next_phase;
    }

    /* Real hardware assertion: the drive is now sitting at its physical
     * clamp (track 34, quarter-track 139) -- verify our own clamp
     * engaged correctly first (this part was never in question). */
    assert(ctl.drive[0].track == (DISK2_MAX_TRACKS * 4 - 1));

    /* THE REAL TEST: a real disk2_controller_access() bus-level address-
     * field read at this overshot/clamped position must succeed and
     * report the REAL track number (34) -- exactly what a real Disk II
     * head sitting at its mechanical stop would report, letting DOS
     * 3.3's real RDRIGHT/SETTRK self-correction mechanism (Beneath
     * Apple DOS, $BDED-$E03/$BE95) trigger and recover, instead of
     * hanging forever. */
    ctl.drive[0].head = 0;
    seek_to_prolog(&ctl, 0xD5, 0xAA, 0x96);
    uint8_t volume = decode_44(&ctl);
    uint8_t track = decode_44(&ctl);
    uint8_t sector = decode_44(&ctl);
    uint8_t checksum = decode_44(&ctl);

    if ((uint8_t)(volume ^ track ^ sector) != checksum) {
        fprintf(stderr, "FAIL: address field checksum mismatch after seek overshoot "
                "(vol=%u track=%u sector=%u chk=%u) -- real hardware would never "
                "corrupt a real disk's address field just because a seek overshot\n",
                volume, track, sector, checksum);
        assert(0);
    }
    if (track != DISK2_MAX_TRACKS - 1) {
        fprintf(stderr, "FAIL: address field reports track=%u after seek overshoot, "
                "expected the real clamped track %u -- this is the exact defect that "
                "prevents DOS 3.3's real SETTRK self-correction from ever triggering\n",
                track, DISK2_MAX_TRACKS - 1);
        assert(0);
    }

    printf("PASS: address-field read at seek-overshoot clamp position correctly "
           "reports the real clamped track number (matches real Disk II hardware "
           "hard-stop behavior)\n");
    printf("All tests passed.\n");
    return 0;
}
