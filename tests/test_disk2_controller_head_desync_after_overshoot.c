/*
 * tests/test_disk2_controller_head_desync_after_overshoot.c -- RED
 * test for hypothesis #1 from NEXT_STEPS.md's "DUKE'S TDD FIX ATTEMPT"
 * section: does the real live-boot disk-swap RWTS hang stem from
 * d->head landing at a position, after realistic overshoot-seek +
 * interleaved-read activity, from which a sync-mark search genuinely
 * cannot converge within a realistic bound?
 *
 * Real Beneath Apple DOS finding (RDADR routine, $B944-$B99F): RDADR
 * itself has NO documented internal byte-count timeout -- it scans
 * until it finds a real D5-AA-96 prolog, however long that takes. The
 * OUTER retry count (48, from TRYTRK/$BDBC-$BDEC) is what's bounded,
 * with each retry being one full RDADR attempt. Since any real disk
 * track has a sync mark roughly every ~400-450 nibbles (post-gap
 * sector spacing), a single RDADR attempt from ANY starting head
 * position should converge within at most one full track's worth of
 * nibbles (~6650, this project's real per-track nibble count) --
 * this test checks that bound directly, simulating a realistic
 * post-overshoot head position (NOT reset to 0, unlike the prior,
 * disproven hypothesis's test).
 */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../src/disk2_controller.h"
#include "../src/cpu6502.h"

#define LOC_DRIVEON 0x9
#define LOC_DRIVEREAD 0xC
#define LOC_DRIVEREADMODE 0xE
#define LOC_PHASE0ON 0x1
#define LOC_PHASE1ON 0x3
#define LOC_PHASE2ON 0x5
#define LOC_PHASE3ON 0x7

static uint8_t read_one_nibble(disk2_controller_t *ctl, long *nibbles_consumed) {
    uint8_t b;
    long attempts = 0;
    do {
        b = disk2_controller_access(ctl, LOC_DRIVEREAD, 0, 0);
        if ((b & 0x80) == 0) {
            clockticks6502 += 32;
            attempts++;
            /* Safety valve for the test itself -- if nibble_shift() ever
             * fails to advance at all (a genuinely different bug), don't
             * hang the test suite forever. */
            if (attempts > 1000000) {
                fprintf(stderr, "FAIL: read_one_nibble() never returned a valid "
                        "nibble after 1,000,000 attempts -- nibble_shift() itself "
                        "appears stuck, not just slow to find a sync mark\n");
                assert(0);
            }
        }
    } while ((b & 0x80) == 0);
    if (nibbles_consumed) (*nibbles_consumed)++;
    return b;
}

/* Scans for the given 3-byte prolog, counting how many real nibbles
 * were consumed before it was found -- this is the real, direct
 * measurement of "how far away is the next sync mark from wherever
 * d->head happens to be". */
static long scan_for_prolog(disk2_controller_t *ctl, uint8_t p0, uint8_t p1, uint8_t p2,
                             long give_up_after_nibbles) {
    uint8_t window[3] = {0, 0, 0};
    long consumed = 0;
    for (;;) {
        long dummy = 0;
        uint8_t b = read_one_nibble(ctl, &dummy);
        consumed++;
        window[0] = window[1];
        window[1] = window[2];
        window[2] = b;
        if (window[0] == p0 && window[1] == p1 && window[2] == p2) {
            return consumed;
        }
        if (consumed > give_up_after_nibbles) {
            return -1; /* not found within bound */
        }
    }
}

int main(void) {
    extern void encode_sector_16_for_test(uint8_t volume, uint8_t track, uint8_t sector,
                                           const uint8_t data[256], uint8_t *out, int *out_len);

    /* Build a REALISTIC full track: 16 sectors, same as a real 4am-
     * cracked Zork/DOS 3.3 disk track 34 (the physical clamp track),
     * not a single synthetic sector -- this matters because it gives
     * the test real, evenly-spaced sync marks to search across,
     * matching the real live-boot scenario's disk layout exactly. */
    static uint8_t track_nibbles[8192];
    int track_len = 0;
    static uint8_t sector_data[256];
    int p = 0;
    for (int sec = 0; sec < 16 && p < 8000; sec++) {
        for (int i = 0; i < 256; i++) {
            sector_data[i] = (uint8_t)((i * 13 + sec * 7) & 0xFF);
        }
        uint8_t one_sector[600];
        int one_len = 0;
        encode_sector_16_for_test(254, DISK2_MAX_TRACKS - 1, (uint8_t)sec, sector_data, one_sector, &one_len);
        memcpy(track_nibbles + p, one_sector, (size_t)one_len);
        p += one_len;
    }
    track_len = p;

    disk2_controller_t ctl;
    disk2_controller_reset(&ctl);

    static disk2_nibble_track_t tracks[DISK2_MAX_TRACKS];
    memset(tracks, 0, sizeof(tracks));
    memcpy(tracks[DISK2_MAX_TRACKS - 1].data, track_nibbles, (size_t)track_len);
    tracks[DISK2_MAX_TRACKS - 1].length = track_len;
    disk2_controller_load_nibble_disk(&ctl, 0, tracks, 0);

    disk2_controller_access(&ctl, LOC_DRIVEON, 1, 0);
    disk2_controller_access(&ctl, LOC_DRIVEREADMODE, 1, 0);

    /* Drive to the clamp via real overshoot phase-stepping (same as
     * the prior test). */
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
    assert(ctl.drive[0].track == (DISK2_MAX_TRACKS * 4 - 1));

    /* THE KEY DIFFERENCE from the disproven hypothesis's test: instead
     * of resetting d->head to 0 (a clean start), simulate a REALISTIC
     * accumulated head position by consuming a large, arbitrary,
     * non-aligned number of real nibbles first -- as would happen from
     * many interleaved real read attempts during a long overshoot-retry
     * sequence in the actual live boot. Test at SEVERAL different
     * arbitrary starting offsets to see if convergence ever fails,
     * not just one lucky/unlucky position. */
    int start_offsets[] = {0, 137, 1000, 3333, 5000, 6601};
    int num_offsets = (int)(sizeof(start_offsets) / sizeof(start_offsets[0]));
    long worst_case_nibbles = 0;

    for (int oi = 0; oi < num_offsets; oi++) {
        ctl.drive[0].head = start_offsets[oi] % tracks[DISK2_MAX_TRACKS - 1].length;

        /* Real-world bound: one full track's worth of nibbles is the
         * natural upper limit for finding ANY sync mark on a real disk
         * (sync marks repeat roughly every ~400-450 bytes; a full
         * track guarantees passing at least one, usually many). Real
         * RWTS's RDADR has no smaller internal timeout per Beneath
         * Apple DOS -- it scans until found. */
        long bound = tracks[DISK2_MAX_TRACKS - 1].length + 100; /* small safety margin */
        long found_at = scan_for_prolog(&ctl, 0xD5, 0xAA, 0x96, bound);

        if (found_at < 0) {
            fprintf(stderr, "FAIL: starting from head=%d, no D5-AA-96 sync mark found "
                    "within %ld nibbles (a full track's worth) -- this IS the real bug: "
                    "the sync search cannot converge from this starting position, exactly "
                    "matching the live-boot symptom of RWTS retrying forever without ever "
                    "reading a real address field\n",
                    start_offsets[oi] % tracks[DISK2_MAX_TRACKS - 1].length, bound);
            assert(0);
        }
        if (found_at > worst_case_nibbles) worst_case_nibbles = found_at;
        fprintf(stderr, "  start_head=%d -> found sync mark after %ld nibbles\n",
                start_offsets[oi] % tracks[DISK2_MAX_TRACKS - 1].length, found_at);
    }

    fprintf(stderr, "Worst case across %d starting positions: %ld nibbles to find sync\n",
            num_offsets, worst_case_nibbles);
    printf("PASS: sync-mark search converges from every tested starting head position "
           "within one track's worth of nibbles\n");
    printf("All tests passed.\n");
    return 0;
}
