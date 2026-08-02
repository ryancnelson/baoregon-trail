/*
 * tools/debug_zork1_retry_loop.c -- reproduces fable-5's emu_trace
 * finding (dd86358): real Zork I boot gets stuck cycling among a small
 * fixed set of addresses ($2602/$2605/$254F/$2548/$2552/$257C) for
 * billions of cycles with zero progress. This tool boots the exact
 * same way main_qemu_zork1boot.c does (same system ROM patches, same
 * embedded nibble disk data, same $C600 entry point/calling convention)
 * but on the HOST (fast, LLDB-debuggable) instead of inside QEMU, and
 * dumps disassembly + disk2_controller_t state whenever PC visits one
 * of the reported loop addresses, to find the actual root cause fast.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../src/apple2_mem.h"
#include "../src/cpu6502.h"
#include "../src/disk2_controller.h"
#include "../src/zork1_nib_disk_data.h"
#include "../src/apple2e_system_rom.h"

#define SYSTEM_ROM_SIZE 16384
static uint8_t g_system_rom[SYSTEM_ROM_SIZE];

static void init_system_rom(void) {
    for (int i = 0; i < SYSTEM_ROM_SIZE; i++) {
        g_system_rom[i] = g_apple2e_system_rom[i];
    }
    g_system_rom[0x2000] = 0x4C; g_system_rom[0x2001] = 0x00; g_system_rom[0x2002] = 0xE0;
    g_system_rom[0x3F58] = 0x60;
    g_system_rom[0x3E89] = 0x60;
    g_system_rom[0x3E93] = 0x60;
    g_system_rom[0x3B2F] = 0x60;
    g_system_rom[0x388E] = 0x60;
    g_system_rom[0x3CA8] = 0xA9; g_system_rom[0x3CA9] = 0x00; g_system_rom[0x3CAA] = 0x60;
}

static disk2_nibble_track_t g_tracks[DISK2_MAX_TRACKS];

static void load_embedded_nib_disk(void) {
    for (int t = 0; t < G_ZORK1_TRACKS_NUM_TRACKS; t++) {
        for (int b = 0; b < G_ZORK1_TRACKS_MAX_TRACK_BYTES; b++) {
            g_tracks[t].data[b] = g_zork1_tracks_track_data[t][b];
        }
        g_tracks[t].length = g_zork1_tracks_track_lengths[t];
    }
}

/* The 6 loop addresses fable-5's emu_trace found (dd86358). */
static const uint16_t LOOP_ADDRS[] = {0x2602, 0x2605, 0x254F, 0x2548, 0x2552, 0x257C};
#define NUM_LOOP_ADDRS (sizeof(LOOP_ADDRS) / sizeof(LOOP_ADDRS[0]))

static int is_loop_addr(uint16_t addr) {
    for (size_t i = 0; i < NUM_LOOP_ADDRS; i++) {
        if (LOOP_ADDRS[i] == addr) return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    uint32_t cycles = (argc > 1) ? (uint32_t)atol(argv[1]) : 20000000u;

    init_system_rom();
    load_embedded_nib_disk();

    apple2_mem_reset();
    reset6502();
    apple2_mem_load_system_rom(g_system_rom);
    apple2_mem_set_disk_controller_mode(APPLE2_MEM_DISK_CONTROLLER_DISK2);
    disk2_controller_t *ctl = apple2_mem_get_disk2_controller();
    disk2_controller_load_nibble_disk(ctl, 0, g_tracks, 0);

    pc = 0xC600;
    x = 0x60;
    a = 0x60;

    /* Dump disassembly bytes around each loop address up front, before
     * any execution -- since this is data read from the disk image
     * (RWTS boot code, not our own boot ROM), we need to see what the
     * actual disk-supplied bytes are at those addresses once loaded
     * into RAM by the boot sequence. We can't dump them yet (RAM is
     * empty pre-boot) -- print them after a short run instead. */

    uint32_t total_executed = 0;
    uint32_t chunk = 1;
    uint16_t last_pc = pc;
    int stuck_count = 0;
    long loop_addr_hits[NUM_LOOP_ADDRS];
    memset(loop_addr_hits, 0, sizeof(loop_addr_hits));
    static long pc_histogram[0x300];
    memset(pc_histogram, 0, sizeof(pc_histogram));
    uint32_t last_seek_log = 0;
    int seek_check_count = 0;
    int dumped_context = 0;

    while (total_executed < cycles) {
        uint16_t pc_before = pc;
        exec6502(chunk);
        total_executed += chunk;

        if (pc == pc_before) {
            stuck_count++;
        } else {
            stuck_count = 0;
        }
        last_pc = pc_before;

        if (pc_before >= 0x2400 && pc_before < 0x2700) {
            pc_histogram[pc_before - 0x2400]++;
        }

        /* Log track-seek convergence every 200K cycles: is $0478
         * (current-track byte per RWTS convention) ever actually
         * reaching the target track, or does the seek routine loop
         * forever without converging? */
        if (total_executed - last_seek_log >= 200000) {
            last_seek_log = total_executed;
            fprintf(stderr, "[seek] cyc=%u pc=$%04X ctl.track(qtr)=%d $0478=%d(0x%02X) a=%02X\n",
                    total_executed, pc, ctl->drive[0].track, read6502(0x0478), read6502(0x0478), a);
        }

        /* Catch the seek convergence check itself: pc=$25A4 is CMP $0478
         * (A = target track). Log A vs $0478 and whether BEQ ($25A7)
         * would actually fire, every time this exact instruction runs
         * (not rate-limited -- want every attempt). */
        if (pc_before == 0x25A4 && seek_check_count < 200) {
            seek_check_count++;
            uint8_t cur_track_byte = read6502(0x0478);
            fprintf(stderr, "[seek-check #%d] target_a=%02X cur_track($0478)=%02X match=%d cyc=%u caller_ret=$%04X\n",
                    seek_check_count, a, cur_track_byte, a == cur_track_byte, total_executed,
                    (uint16_t)(read6502((uint16_t)(sp + 1 + 0x100)) | (read6502((uint16_t)(sp + 2 + 0x100)) << 8)));
        }

        /* Real candidate root cause per disk2_controller.c's own file
         * header note: "Write mode + Q6 shift: not yet implemented".
         * If Zork's boot/copy-protection code ever engages Q7 (write
         * mode) as part of a self-check, our controller silently no-ops
         * instead of performing a real write -- log every q7 transition
         * to confirm/rule this out. */
        /* Track head wraparound events -- if disk2_controller.c's
         * `(d->head + nibbles) % track->length` ever produces an
         * off-by-one or skips a byte at the wrap boundary, address-field
         * search would intermittently miss syncs near the end of the
         * track, causing exactly this kind of "almost working, retries
         * forever" symptom. */
        static int last_head = -1;
        int cur_head = ctl->drive[0].head;
        if (last_head >= 0 && cur_head < last_head && (last_head - cur_head) > (6632 / 2)) {
            static int wrap_count = 0;
            wrap_count++;
            if (wrap_count <= 10) {
                fprintf(stderr, "[wrap #%d] head %d -> %d (track len context) cyc=%u\n", wrap_count, last_head, cur_head, total_executed);
            }
        }
        last_head = cur_head;

        static int last_motor = -1;
        static int last_selected = -1;
        if (ctl->motor_on != last_motor || ctl->selected_drive != last_selected) {
            static int motor_change_count = 0;
            motor_change_count++;
            if (motor_change_count <= 20) {
                fprintf(stderr, "[motor/drive change #%d] motor: %d->%d sel: %d->%d pc=$%04X cyc=%u\n",
                        motor_change_count, last_motor, ctl->motor_on, last_selected, ctl->selected_drive,
                        pc_before, total_executed);
            }
            last_motor = ctl->motor_on;
            last_selected = ctl->selected_drive;
        }

        /* Track every real disk2_controller_access() phase-changing
         * call during the seek, to see if the emulator's own
         * ctl->drive[0].track ever converges toward a stable target,
         * or if it's endlessly oscillating (a real controller bug)
         * vs. Zork's own code simply retrying a legitimately-failing
         * higher-level check unrelated to our phase-stepping. */
        static int last_track = -1;
        static int track_change_count = 0;
        if (ctl->drive[0].track != last_track) {
            track_change_count++;
            if (track_change_count <= 60) {
                fprintf(stderr, "[track change #%d] track(qtr): %d -> %d at pc=$%04X cyc=%u\n",
                        track_change_count, last_track, ctl->drive[0].track, pc_before, total_executed);
            }
            last_track = ctl->drive[0].track;
        }

        for (size_t i = 0; i < NUM_LOOP_ADDRS; i++) {
            if (LOOP_ADDRS[i] == pc_before) {
                loop_addr_hits[i]++;
                if (loop_addr_hits[i] <= 3 || (loop_addr_hits[i] % 500000) == 0) {
                    fprintf(stderr, "[hit #%ld] pc=$%04X a=$%02X x=$%02X y=$%02X status=$%02X cyc=%u\n",
                            loop_addr_hits[i], pc_before, a, x, y, status, total_executed);
                }
            }
        }

        /* Once we've accumulated real hits on the loop addresses (proves
         * we've reached the reported stuck state), dump full context:
         * raw bytes at/around each loop address, disk2_controller_t
         * state, and A/X/Y/status at that moment. */
        if (!dumped_context && loop_addr_hits[0] > 1000 && loop_addr_hits[1] > 1000) {
            dumped_context = 1;
            fprintf(stderr, "=== Reached loop state after %u cycles ===\n", total_executed);
            fprintf(stderr, "CPU: pc=$%04X a=$%02X x=$%02X y=$%02X sp=$%02X status=$%02X\n",
                    pc, a, x, y, sp, status);
            fprintf(stderr, "Wide disasm window $2A00-$2B00:\n");
            for (uint16_t addr = 0x2A00; addr <= 0x2B00; addr += 16) {
                fprintf(stderr, "$%04X: ", addr);
                for (int b = 0; b < 16; b++) {
                    fprintf(stderr, "%02X ", read6502((uint16_t)(addr + b)));
                }
                fprintf(stderr, "\n");
            }
            fprintf(stderr, "Wide disasm window $2300-$2480:\n");
            for (uint16_t addr = 0x2300; addr <= 0x2480; addr += 16) {
                fprintf(stderr, "$%04X: ", addr);
                for (int b = 0; b < 16; b++) {
                    fprintf(stderr, "%02X ", read6502((uint16_t)(addr + b)));
                }
                fprintf(stderr, "\n");
            }
            fprintf(stderr, "Zero page $26-$2B, $46-$47, $78 (track/seek state candidates):\n");
            fprintf(stderr, "  $26=%02X $27=%02X $28=%02X $29=%02X $2A=%02X $2B=%02X $46=%02X $47=%02X $78=%02X $79=%02X\n",
                    read6502(0x26), read6502(0x27), read6502(0x28), read6502(0x29),
                    read6502(0x2A), read6502(0x2B), read6502(0x46), read6502(0x47),
                    read6502(0x478 & 0xFF), read6502((0x478 >> 8) & 0xFF));
            fprintf(stderr, "  $0478 (16-bit? absolute)=%02X\n", read6502(0x0478));
            for (size_t i = 0; i < NUM_LOOP_ADDRS; i++) {
                uint16_t base = LOOP_ADDRS[i];
                fprintf(stderr, "Bytes at $%04X-$%04X: ", base, (uint16_t)(base + 7));
                for (int b = 0; b < 8; b++) {
                    fprintf(stderr, "%02X ", read6502((uint16_t)(base + b)));
                }
                fprintf(stderr, "\n");
            }
            fprintf(stderr, "disk2 controller: motor_on=%d q6=%d q7=%d selected_drive=%d latch=0x%02X\n",
                    ctl->motor_on, ctl->q6, ctl->q7, ctl->selected_drive, ctl->latch);
            fprintf(stderr, "drive[0]: track=%d head=%d skip=%d has_disk=%d\n",
                    ctl->drive[0].track, ctl->drive[0].head, ctl->drive[0].skip,
                    ctl->drive[0].has_disk);
            /* Dump a window of nibble bytes around the current head
             * position -- if we're stuck retrying a checksum, the bytes
             * right at/before head are the address/data field being
             * (mis-)verified. */
            int track_idx = ctl->drive[0].track / 4; /* quarter-track -> whole track */
            if (track_idx >= 0 && track_idx < DISK2_MAX_TRACKS) {
                int head = ctl->drive[0].head;
                int len = g_tracks[track_idx].length;
                fprintf(stderr, "Track %d (len=%d) nibble window around head=%d:\n", track_idx, len, head);
                for (int off = -8; off <= 8; off++) {
                    int idx = head + off;
                    if (idx < 0) idx += len;
                    if (idx >= len) idx -= len;
                    fprintf(stderr, "%02X ", g_tracks[track_idx].data[idx]);
                }
                fprintf(stderr, "\n");
            }
        }

        if (stuck_count > 20000000) {
            fprintf(stderr, "Hard-stuck (single PC never changes) at $%04X after %u cycles\n", last_pc, total_executed);
            break;
        }
    }

    fprintf(stderr, "\n=== Final summary after %u cycles ===\n", total_executed);
    for (size_t i = 0; i < NUM_LOOP_ADDRS; i++) {
        fprintf(stderr, "  $%04X: %ld hits\n", LOOP_ADDRS[i], loop_addr_hits[i]);
    }
    fprintf(stderr, "Final PC=$%04X A=$%02X X=$%02X Y=$%02X\n", pc, a, x, y);
    fprintf(stderr, "Final disk2 state: track=%d head=%d skip=%d motor_on=%d latch=0x%02X\n",
            ctl->drive[0].track, ctl->drive[0].head, ctl->drive[0].skip, ctl->motor_on, ctl->latch);

    fprintf(stderr, "\nPC histogram $2400-$2700 (addresses with >1000 hits):\n");
    for (int i = 0; i < 0x300; i++) {
        if (pc_histogram[i] > 1000) {
            fprintf(stderr, "  $%04X: %ld\n", 0x2400 + i, pc_histogram[i]);
        }
    }

    return 0;
}
