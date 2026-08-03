# Reference-Boot Cross-Check: reinette-II-plus Native macOS Build vs Our Zork I Bug

Woz, 2026-08-03 (continuation of the same investigation), for Duke's
disk-swap/RWTS investigation. **MAJOR CORRECTION to the original
version of this doc (commit `1f734e4` on `spike-reinette-port`) --
the original finding was a methodology error. See "Correction" section
below before reading anything else.**

## Correction (read this first)

The original session tested `~/Downloads/Zork_I.dsk` -- a **protected,
NOT-yet-cracked** Zork I disk image, not the actual working file this
project uses (`tools/zork1_4amcrack.dsk`, 4am's protection-removal
crack). Confirmed the mixup directly: the real, protected `Zork_I.dsk`
contains a `CMP #$BC` instruction (raw byte offset 763) that searches
for a nonstandard `D5 AA BC` sync marker as part of its copy-protection
scheme -- a marker that genuinely does not exist anywhere in a
standard/generic nibblized image (confirmed zero occurrences across
all 35 tracks, in both my own `.nib` conversion AND this project's own
pre-existing, independently-verified `src/zork1_nib_disk_data.h`). That
protected disk's custom bootloader will NEVER succeed against a
standard RWTS-style nibblization (ours or anyone else's) -- it's a real,
genuinely-hard-for-any-naive-nibblizer disk, but it is **NOT the disk
this project has ever actually used or is trying to boot**. `tools/
zork1_4amcrack.dsk` has ZERO `CMP #$BC` occurrences (confirmed via the
same byte-search) -- its whole point is that the protection/custom sync
scheme has been removed, replaced with standard RWTS-compatible
encoding.

## Corrected, real result: reinette DOES boot the real, correct disk

Re-ran the SAME diagnostic-patched reinette-II-plus native build
(audio-hang + Metal-vsync-stall workarounds, see below) against
`tools/zork1_4amcrack.dsk` -- converted via
`tools/build_nib_for_reference_emu.py` (already committed on `main`),
same tool, same process, just the CORRECT source file this time.

**Result: complete, genuine SUCCESS.** Screen memory (verified via lldb
`memory read` against the live process, bypassing this environment's
separate, unrelated screen-rendering stall) shows real, readable Zork I
game text -- confirmed via TWO separate live process launches
(different runs, different ASLR-shifted addresses, byte-identical
decoded content both times):
```
WEST OF HOUSE
ZORK: THE GREAT UNDERGROUND EMPIRE - PART I
COPYRIGHT (C) 1980 BY INFOCOM, INC. ALL RIGHTS RESERVED.
YOU ARE STANDING IN AN OPEN FIELD WEST OF A WHITE HOUSE, WITH A BOARDED
FRONT DOOR.
THERE IS A SMALL MAILBOX HERE.
ZORK IS A TRADEMARK OF INFOCOM, INC.
RELEASE 15 / SERIAL NUMBER UG3AU5
```
Plus a live `>` prompt character visible in the dump. This is a real,
playable Zork I boot -- not garbled, not a partial banner, and not a
screenshot artifact (the visual `hs.window:snapshot()` screenshot
IS black/broken in this environment, per the same known Metal
`nextDrawable` stall noted below -- the lldb memory read is the only
evidence being relied on here, confirmed explicitly not to be
conflated with the unreliable screenshot). Confirmed stable
(byte-identical across live samples ~5 seconds apart within the same
run, and across two independent fresh process launches), process CPU
usage settled to near-idle ~1.7%, consistent with a real game waiting
at its input prompt, not busy-looping or crashed.

## What this means

This REVERSES the original finding. reinette-II-plus's independent
6502 core + independent Disk][ nibble implementation boots the actual,
real disk image this project uses (`zork1_4amcrack.dsk`) successfully,
with the SAME `.dsk`-\>`.nib` conversion tooling this project already
has. This is genuinely useful, different information than the original
(wrong-disk) finding:

- It does NOT support "this disk image is inherently too hard for any
  naive nibble emulation" -- that theory was built on testing the wrong
  file.
- It DOES support: whatever is stalling OUR OWN emulator
  (`disk2_controller.c`) on this same disk is a real, specific bug in
  OUR implementation (or in how our own boot harness drives it), since
  an independent implementation with a COMPLETELY different (simpler,
  untimed) disk-read model boots the SAME real data to real game text
  without issue.
- reinette's `$C0EC` disk-latch handler has NO elapsed-cycle timing
  gating at all -- every access immediately returns the next sequential
  nibble (see `case 0xC0EC` in `reinetteII+.c`). This is even simpler
  than the apple2js `skip`-toggle model looked at earlier this session.
  If the real bug in our own `nibble_shift()` is a timing-model
  mismatch, this is a THIRD independent confirmation (apple2js,
  reinette, and implicitly "no timing gating at all") that timing
  models other than ours succeed on real disk data -- worth revisiting
  whether `nibble_shift()`'s elapsed-cycle math needs to exist at all,
  versus a simpler model.

## Diagnostic method (unchanged from original, still valid)

Got past reinette-II-plus's native macOS SDL2 build's
`SDL_OpenAudioDevice()` hang (confirmed environment-specific, not a
reinette bug) and a separate Metal `nextDrawable` semaphore stall
(confirmed via `lldb` backtrace, blocks real window rendering for many
real seconds per frame in this specific terminal-launched environment)
via `/tmp/reinette_noaudio/` scratch patches:
- Skip `SDL_OpenAudioDevice()` entirely.
- Drop `SDL_RENDERER_PRESENTVSYNC`.
- (New this pass) Added disk-read instrumentation logging every $C0EC
  access's PC/track/nibble-offset/value to `/tmp/reinette_disk_trace.log`,
  used to disassemble and understand exactly where/why the WRONG-disk
  test got stuck (the real, useful diagnostic value of that mistake --
  it's a genuine, correct trace of a genuinely-unbootable protected
  disk, just not the disk we care about).

Verification method: lldb `process attach` + direct RAM memory reads of
the running process (`memory read --format y --size 1 --count 960
--outfile <path> <addr>` against `&ram + 0x400`, runtime address via
`expr -f x -- (unsigned long)&ram + 0x400` after attaching -- ASLR
shifts it per-run). Screenshots (`hs.window:snapshot()`) are unreliable
in this environment due to the separate Metal stall noted above --
memory reads are the reliable ground truth, not screenshots.

## Files

- `/tmp/reinette_noaudio/reinette_trace` -- diagnostic-patched build
  (audio+vsync skip, disk-read PC/track/offset/value logging), scratch,
  not committed.
- `/tmp/reinette_nibs2/zork1_4amcrack.nib` -- correct, real disk data
  converted from `tools/zork1_4amcrack.dsk` via
  `tools/build_nib_for_reference_emu.py`.
- `/tmp/reinette_nibs2/zork1_real.nib` -- the WRONG/protected disk
  (from `~/Downloads/Zork_I.dsk`) used in the original, corrected
  finding -- kept for reference since its stall trace is a genuinely
  interesting real example of a hard copy-protection scheme, just not
  relevant to this project's actual bug.
- `/tmp/reinette_disk_trace.log` -- most recent trace is against the
  WRONG disk (for the record/reproducibility of that side-finding);
  rerun against `zork1_4amcrack.nib` if a trace of the CORRECT boot is
  wanted (it completes fast enough that a 200000-line cap won't even
  trigger).

## Handoff

Real, actionable conclusion for Duke: an independent 6502 + Disk][
implementation boots the actual, correct `zork1_4amcrack.dsk` data to
real game text using a MUCH simpler (no elapsed-cycle timing at all)
nibble-read model than our own `nibble_shift()`. Worth testing directly
whether removing/simplifying our own timing-gating model produces the
same real success, as a genuine repair hypothesis rather than just
"here's ANOTHER piece of corroborating-but-inconclusive evidence."

Note on branch state: this doc was originally written from a `/tmp`
scratch location because the checked-out branch had been moved to
`main` mid-session by the ralph-loop automation's own `pull --rebase`
(not by me), and I was holding off on git operations per a standing
instruction not to interrupt `spike-reinette-port` work elsewhere.
Ryan confirmed directly (2026-08-03) that no rebase is in progress
anywhere and both branches are clean -- landing this properly on
`main` now as `NEXT_STEPS_REINETTE_ZORK_CROSSCHECK.md`, cross-referenced
from `NEXT_STEPS.md`. The original (now-superseded) version lives at
`spike-reinette-port`'s commit `1f734e4` -- same filename -- and should
be treated as corrected/superseded by this version if anyone lands on
that branch later.
