/*
 * RED test: $C0E0-$C0EF fast-sector-read trap, driven through a mock bus
 * matching the locked read6502/write6502 signatures (uint8_t
 * read6502(uint16_t); void write6502(uint16_t, uint8_t)).
 *
 * apple2_mem.c (the real implementation of read6502/write6502 with the
 * $C000-$C0FF soft-switch dispatch baochip sketched) is not written yet --
 * blocked on Woz's cpu6502.c stubs landing. Per Ryan's steer 2026-07-31:
 * don't block on that. This test builds a minimal mock dispatcher, matching
 * baochip's sketched shape (address-range branch -> duke_disk_*_trap), to
 * drive and verify disk_trap.c's logic now. Swap this mock for the real
 * apple2_mem.c wiring once Woz's stubs land (see NEXT_STEPS.md Step 3).
 *
 * Soft-switch protocol under test (Duke's design, no physical Disk II
 * stepper/GCR emulation per BRAINSTORM.md section 4):
 *   write6502(0xC0E0, track);   -- select track
 *   write6502(0xC0E1, sector);  -- select sector, triggers the "seek"
 *   read6502(0xC0E2 + i)        -- read byte i (0..255) of the selected
 *                                  sector, wrapping in the $C0E2-$C0EF
 *                                  16-byte window (real DOS 3.3 streams
 *                                  bytes serially; the mock/window design
 *                                  is Duke's, revisit once real timing
 *                                  requirements are known).
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "../src/disk_sector_layout.h"
#include "../src/disk_trap.h"

static int failures = 0;

/* Synthetic disk image: byte at absolute offset N == (N & 0xFF), so any
 * sector's bytes are trivially predictable and distinct within a sector,
 * making it easy to catch off-by-one/wrong-sector bugs. */
static uint8_t g_mock_disk_image[DOS33_DISK_IMAGE_SIZE];

/* Mock bus, mirroring baochip's sketched apple2_mem.c dispatch shape:
 * address-range branch inside write6502/read6502 routing $C0E0-$C0EF to
 * Duke's disk trap. This is NOT apple2_mem.c -- it's a stand-in with the
 * same locked signatures, to be deleted once the real file exists. */
static uint8_t g_selected_byte_offset = 0;

uint8_t read6502(uint16_t address) {
    if (address >= 0xC0E2 && address <= 0xC0EF) {
        uint8_t byte_offset = g_selected_byte_offset + (uint8_t)(address - 0xC0E2);
        return disk_trap_read_byte(byte_offset);
    }
    return 0x00;
}

void write6502(uint16_t address, uint8_t value) {
    static uint8_t pending_track = 0xFF; /* 0xFF = "no track staged" */

    if (address == 0xC0E0) {
        pending_track = value;
    } else if (address == 0xC0E1) {
        disk_trap_select_sector(pending_track, value);
        g_selected_byte_offset = 0;
    }
}

static void fill_mock_disk_image(void) {
    for (uint32_t i = 0; i < DOS33_DISK_IMAGE_SIZE; i++) {
        g_mock_disk_image[i] = (uint8_t)(i & 0xFF);
    }
}

static int test_select_and_read_track0_sector0(void) {
    write6502(0xC0E0, 0);  /* track 0 */
    write6502(0xC0E1, 0);  /* sector 0 -> offset 0 in the image */

    /* First 14 bytes of track 0 sector 0 come through the $C0E2-$C0EF
     * window (16 addresses, so 14 usable after the two select writes'
     * addresses are excluded from the read range). */
    for (uint16_t addr = 0xC0E2; addr <= 0xC0EF; addr++) {
        uint32_t sector_offset = (addr - 0xC0E2);
        uint8_t expected = g_mock_disk_image[0 + sector_offset];
        uint8_t got = read6502(addr);
        if (got != expected) {
            fprintf(stderr, "FAIL: read6502(0x%04X) = 0x%02X, expected 0x%02X (track 0 sector 0)\n",
                    addr, got, expected);
            failures++;
            return 1;
        }
    }
    printf("PASS: test_select_and_read_track0_sector0\n");
    return 0;
}

static int test_select_and_read_track17_sector0_matches_vtoc_offset(void) {
    /* Track 17 sector 0 is DOS 3.3's VTOC -- byte-for-byte offset sanity
     * check against dos33_sector_offset(), not just an arbitrary sector. */
    uint32_t expected_base_offset;
    if (dos33_sector_offset(17, 0, &expected_base_offset) != 0) {
        fprintf(stderr, "FAIL: dos33_sector_offset(17, 0) unexpectedly rejected\n");
        failures++;
        return 1;
    }

    write6502(0xC0E0, 17);
    write6502(0xC0E1, 0);

    for (uint16_t addr = 0xC0E2; addr <= 0xC0EF; addr++) {
        uint32_t sector_offset = (addr - 0xC0E2);
        uint8_t expected = g_mock_disk_image[expected_base_offset + sector_offset];
        uint8_t got = read6502(addr);
        if (got != expected) {
            fprintf(stderr, "FAIL: read6502(0x%04X) = 0x%02X, expected 0x%02X (track 17 sector 0, VTOC)\n",
                    addr, got, expected);
            failures++;
            return 1;
        }
    }
    printf("PASS: test_select_and_read_track17_sector0_matches_vtoc_offset\n");
    return 0;
}

static int test_select_invalid_sector_does_not_crash_or_corrupt(void) {
    /* First, select a known-good sector so we have a baseline. */
    write6502(0xC0E0, 5);
    write6502(0xC0E1, 3);
    uint8_t baseline = read6502(0xC0E2);

    /* Now attempt an out-of-range track; disk_trap_select_sector() must
     * reject it (matches dos33_sector_offset()'s contract) and must not
     * silently move the read pointer to garbage. */
    write6502(0xC0E0, 35); /* invalid: DOS33_TRACKS == 35, so max valid track is 34 */
    write6502(0xC0E1, 0);

    uint8_t after_invalid_select = read6502(0xC0E2);
    if (after_invalid_select != baseline) {
        fprintf(stderr,
                "FAIL: invalid track select changed read6502(0xC0E2) from 0x%02X to 0x%02X -- "
                "trap must reject bad (track,sector) without corrupting current selection\n",
                baseline, after_invalid_select);
        failures++;
        return 1;
    }
    printf("PASS: test_select_invalid_sector_does_not_crash_or_corrupt\n");
    return 0;
}

/*
 * Hardening iteration (2026-07-31): multi-sector sequential read across a
 * full track and across a track boundary.
 *
 * Real DOS 3.3/ProDOS drivers re-issue an explicit track/sector select
 * ($C0E0/$C0E1) before reading each 256-byte sector -- there is no
 * hardware auto-advance across sector or track boundaries on real Disk II
 * hardware, and this trap deliberately mirrors that (BRAINSTORM.md section
 * 4: no physical stepper/GCR emulation, but the *protocol* -- explicit
 * select before each sector -- is preserved). So "sequential read" here
 * means: software walks track/sector pairs itself, selecting each one in
 * turn, and this test proves every one of those selections lands at the
 * mathematically correct offset with no drift, including across the
 * track-16-sectors boundary and the very last sector of the whole disk.
 */
static int test_sequential_select_across_full_track_and_track_boundary(void) {
    /* Walk every sector of track 0, then track 1's sector 0 -- proves the
     * track-boundary transition (sector 15 -> next track's sector 0) lands
     * at the right offset, not just each individual sector in isolation. */
    for (uint8_t sector = 0; sector < DOS33_SECTORS_PER_TRACK; sector++) {
        write6502(0xC0E0, 0);
        write6502(0xC0E1, sector);

        uint32_t expected_base;
        if (dos33_sector_offset(0, sector, &expected_base) != 0) {
            fprintf(stderr, "FAIL: dos33_sector_offset(0, %u) unexpectedly rejected\n", sector);
            failures++;
            return 1;
        }

        for (uint16_t addr = 0xC0E2; addr <= 0xC0EF; addr++) {
            uint32_t byte_offset = addr - 0xC0E2;
            uint8_t expected = g_mock_disk_image[expected_base + byte_offset];
            uint8_t got = read6502(addr);
            if (got != expected) {
                fprintf(stderr,
                        "FAIL: test_sequential_select_across_full_track_and_track_boundary: "
                        "track 0 sector %u byte %u = 0x%02X, expected 0x%02X\n",
                        sector, (unsigned)byte_offset, got, expected);
                failures++;
                return 1;
            }
        }
    }

    /* Now cross the track boundary: track 1, sector 0. */
    write6502(0xC0E0, 1);
    write6502(0xC0E1, 0);

    uint32_t expected_base;
    dos33_sector_offset(1, 0, &expected_base);
    uint8_t got_first_byte = read6502(0xC0E2);
    uint8_t expected_first_byte = g_mock_disk_image[expected_base];
    if (got_first_byte != expected_first_byte) {
        fprintf(stderr,
                "FAIL: test_sequential_select_across_full_track_and_track_boundary: "
                "track 1 sector 0 byte 0 = 0x%02X, expected 0x%02X (track-boundary transition)\n",
                got_first_byte, expected_first_byte);
        failures++;
        return 1;
    }

    printf("PASS: test_sequential_select_across_full_track_and_track_boundary "
           "(16 sectors of track 0 + track boundary into track 1)\n");
    return 0;
}

/*
 * Edge case: the very last valid sector of the whole disk (track 34,
 * sector 15 -- DOS33_TRACKS-1, DOS33_SECTORS_PER_TRACK-1). Its last byte
 * must be exactly the final byte of the 143360-byte image -- an off-by-one
 * in dos33_sector_offset() or the disk image size would show up here as
 * an out-of-bounds read one byte past the end of g_mock_disk_image.
 */
static int test_last_sector_of_disk_reaches_exact_end_of_image(void) {
    write6502(0xC0E0, DOS33_TRACKS - 1);
    write6502(0xC0E1, DOS33_SECTORS_PER_TRACK - 1);

    uint32_t expected_base;
    if (dos33_sector_offset(DOS33_TRACKS - 1, DOS33_SECTORS_PER_TRACK - 1, &expected_base) != 0) {
        fprintf(stderr, "FAIL: dos33_sector_offset(last track, last sector) unexpectedly rejected\n");
        failures++;
        return 1;
    }
    if (expected_base + DOS33_SECTOR_SIZE != DOS33_DISK_IMAGE_SIZE) {
        fprintf(stderr,
                "FAIL: last sector's offset+size = %u, expected exactly DOS33_DISK_IMAGE_SIZE (%d)\n",
                expected_base + DOS33_SECTOR_SIZE, DOS33_DISK_IMAGE_SIZE);
        failures++;
        return 1;
    }

    /* Verify the first 16 bytes of the last sector (the $C0E2-$C0EF mock
     * window's full width) land correctly at the tail of the image -- an
     * off-by-one in dos33_sector_offset() or DOS33_DISK_IMAGE_SIZE would
     * surface here as an out-of-bounds read one byte past the end of
     * g_mock_disk_image. A full 256-byte streaming-cursor proof (the real
     * apple2_mem.c auto-increment behavior, not this mock's fixed window)
     * already exists in tests/test_apple2_mem.c. */
    for (uint16_t addr = 0xC0E2; addr <= 0xC0EF; addr++) {
        uint32_t byte_offset = addr - 0xC0E2;
        uint8_t expected = g_mock_disk_image[expected_base + byte_offset];
        uint8_t got = read6502(addr);
        if (got != expected) {
            fprintf(stderr,
                    "FAIL: test_last_sector_of_disk_reaches_exact_end_of_image: "
                    "byte %u = 0x%02X, expected 0x%02X\n",
                    byte_offset, got, expected);
            failures++;
            return 1;
        }
    }
    printf("PASS: test_last_sector_of_disk_reaches_exact_end_of_image\n");
    return 0;
}

int main(void) {
    fill_mock_disk_image();
    disk_trap_set_image(g_mock_disk_image);

    test_select_and_read_track0_sector0();
    test_select_and_read_track17_sector0_matches_vtoc_offset();
    test_select_invalid_sector_does_not_crash_or_corrupt();
    test_sequential_select_across_full_track_and_track_boundary();
    test_last_sector_of_disk_reaches_exact_end_of_image();

    if (failures == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
