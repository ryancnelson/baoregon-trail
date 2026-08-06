## 🚩 UNRESOLVED CONFLICT (2026-08-03, Woz's 10B-cycle test vs. Duke's 500M-instruction comparison) -- flagged for fresh-context re-verification, NOT reconciled this session

Per Ryan's direction, ran a standalone host harness
(`/tmp/test_zork_bignumber.c`, disposable, not committed) against Zork
I's own real boot sector (via `disk2_controller.c`, real
`zork1_nib_disk_data.h` embedded disk, real `apple2e_system_rom.h`
system ROM patched the same way as `main_qemu_zork1boot.c`) with a
10,000,000,000-cycle budget -- 20x larger than any test run in this
thread before it.

**Result as observed**: every PC value logged across the full 10B
cycles fell within `$254F-$2603` (the nibble-sync-check/address-field
decode loop region disassembled earlier this session). Screen memory
did change sporadically throughout the run (most recently at cycle
8,600,000,000), so the CPU was not frozen/deadlocked -- but this
harness's logging was gated on screen-memory-checksum changes only,
and did not do unconditional periodic PC sampling. This is a real
limitation of the test as run, not verified against a fully
independent, unconditional PC trace before this session ended.

**This appears to conflict with Duke's earlier, separately-documented
finding** (see the "DUKE'S REAL INSTRUCTION-LEVEL 6502 TRACE" and
"DUKE'S 500M-INSTRUCTION FREEZE-POINT VERDICT" entries above), which
used a different, real-time comparison harness
(`tools/reinette_vs_ours_boot_compare.c`, also disposable) and found
concrete evidence of the boot progressing through real track seeks
0-10 and then executing genuine, non-repeating interpreter code past
track 7 (JSR/RTS pairs, indirect zero-page pointer walks, real
arithmetic) -- NOT a stall in the `$254F-$2603` sync-loop region.

**Why this is being flagged rather than reconciled**: this session hit
repeated compaction cycles specifically on this reconciliation
question, which creates real accuracy risk per this tool's own
degraded-context warning. Rather than continue digging in a
context-degraded state and risk producing a confidently-wrong
"resolution," both results are being recorded honestly as-is, with the
conflict flagged explicitly, for a fresh-context session to
re-verify properly.

**Concrete next step for whoever picks this up**: rerun a single,
unified test harness with UNCONDITIONAL periodic PC sampling (not
gated on screen-memory changes) at a fine enough interval (e.g. every
10M cycles) across a 500M-1B cycle budget, using the exact same ROM/
disk setup Duke's comparison harness used, to determine definitively
whether the boot process ever leaves the `$254F-$2603` region. Do not
assume either prior result is correct without this direct
re-verification -- the two existing findings, as currently documented,
are not compatible with each other.

<!-- Woz, 2026-08-03: flagged, not resolved, per explicit direction to stop digging in a context-degraded session -->
