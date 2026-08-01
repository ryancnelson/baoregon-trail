# MAME capture artifacts — provenance and status

Captured 2026-08-01 during a session investigating "dump real Apple II state
from a real emulator, transplant into our own." Preserved here because they
took real effort to produce and may still have value, but **none of them
are currently wired into or used by baoregon-trail's own code** — treat
everything below as raw research material, not verified-working assets.

## Files

### `dos33_boot_ram_dump.bin` (65536 bytes) — GENUINE, VERIFIED
Full 64KB main-RAM snapshot from a real MAME `apple2e` session after
booting `~/Downloads/Apple_DOS_3.3_Master.dsk` for real, using real Apple
IIe ROMs (see `roms/` at repo root) and a real Disk II controller.
**Confirmed real**: text screen memory ($0400-$07FF) contains the literal
string "DOS VERSION 3.3            08/25/80" plus the standard boot banner
text, byte-for-byte matching what a real Apple IIe shows on a real DOS 3.3
boot. This is the single most solid piece of evidence in this whole batch
— it proves the ROMs, disk image, and MAME's Apple IIe driver all work
correctly together.

Produced by: `tools/mame_dump_memory.lua` (committed, reusable) via:
```bash
mame apple2e -video none -sound none -skip_gameinfo -nothrottle -seconds_to_run 30 \
  -flop1 ~/Downloads/Apple_DOS_3.3_Master.dsk \
  -autoboot_script tools/mame_dump_memory.lua
```

### `dos33_boot_registers.txt` — GENUINE, VERIFIED, paired with the RAM dump above
6502 register state (PC, A, X, Y, SP, status flags — decimal, not hex,
known cosmetic bug in the dump script) at the same moment as the RAM dump.
PC=47442 decimal = $B952, deep in DOS's own code — sensible for "just
finished booting, sitting at/near the warmstart loop."

### `rom_c000_ffff_attempt1_A_then_B.bin` (16384 bytes) — FAILED, do not use as-is
My first attempt to reconstruct MAME's real ROM mapping myself, by naively
concatenating `342-0134-a.64` + `342-0135-b.64` in that literal order
(matching the filename lettering). **This is WRONG.** Reset vector reads
back as $4C12, which lands in RAM territory, not a sane ROM entry point —
proof the concatenation order (or possibly the whole "it's just two 8KB
chunks stacked linearly" assumption) is incorrect for how Apple IIe ROM
chips actually map into $C000-$FFFF.

### `rom_c000_ffff_attempt2_mame_live_dump.bin` (16384 bytes) — ALSO INCONCLUSIVE, do not use as-is
Second attempt: instead of guessing concatenation order myself, dumped
MAME's own `:maincpu` program-space memory directly at $C000-$FFFF while
a real boot was live (see `tools/mame_dump_rom.lua`). This should have
been authoritative — MAME's own address space, mid-real-boot — but the
data is ALSO wrong: reset vector reads back as $FF00, and both RESET and
IRQ vectors point to the identical address, which is not how a real Apple
IIe ROM is built. Byte pattern in that region is a suspicious repeating
`00 FF 00 FF...`, suggesting this read hit an unmapped/floating bus region
or a bank-switched/overlaid address range, not the real linear ROM image.

**Root cause NOT FOUND.** Most likely explanation, unconfirmed: Apple IIe's
$C000-$FFFF is not a flat linear ROM in MAME's own address space model —
it likely involves slot-card ROM overlays ($C100-$C7FF for peripheral
cards), the $C800-$CFFF expansion ROM window, and/or bank-switching device
handlers that make a naive `mem:read_u8()` sweep across the whole range
produce something other than the "just read the ROM chip" result you'd
get from, say, `-listroms`/manually inspecting the .64 files with a hex
editor cross-referenced against a known-correct Apple IIe memory map
document. **This needs real investigation before either ROM blob is
trusted for anything.**

### `color_demosoft_hires_dump_INCONCLUSIVE.bin` (8192 bytes) — INCONCLUSIVE, likely stale
Attempted to capture real Hi-Res pixel data from running the disk's
"COLOR DEMOSOFT" program (a genuine Apple demo disk program) after typing
`RUN COLOR DEMOSOFT` + selecting menu option `1` via MAME's
`natkeyboard:post()` API. The captured bytes are byte-for-byte IDENTICAL
between two separate attempts with different keyboard-input timing, which
is a strong signal this is stale/frozen memory (likely still showing
whatever was in the Hi-Res buffer before the demo actually drew anything),
not real pixel output from the program. **Do not use this as "real Apple
II graphics" — it almost certainly isn't.**

## What's actually solid vs. not, one-line summary

| Artifact | Status |
|---|---|
| `dos33_boot_ram_dump.bin` + `.txt` | **Real, verified, trustworthy** |
| Both `rom_c000_ffff_*` attempts | **Both wrong, root cause unknown** |
| `color_demosoft_hires_dump` | **Almost certainly stale, don't trust** |

See `DEVELOPMENT_NOTES.md` at repo root for the full narrative, what was
tried, and concrete next-step milestones for picking this back up.
