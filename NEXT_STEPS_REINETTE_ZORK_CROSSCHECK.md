# Reference-Boot Cross-Check: reinette-II-plus Native macOS Build vs Our Zork I Bug

Woz, 2026-08-02 (late session), for Duke's disk-swap/RWTS investigation.
Standalone findings doc -- NOT editing NEXT_STEPS.md directly to avoid
any collision with Duke's active disk2_controller.c/test file locks.

## What was done

Got past reinette-II-plus's native macOS SDL2 build's earlier
`SDL_OpenAudioDevice()` hang (confirmed environment-specific, not a
reinette bug -- a minimal 15-line standalone SDL2 repro hangs at the
exact same call). Diagnostic-only patch in `/tmp/reinette_noaudio/`
(NOT touching the real vendored source at
`~/devel/apple2-emu-refs/reinette-II-plus/`, NOT touching our RISC-V
port on `spike-reinette-port`):
- Skip `SDL_OpenAudioDevice()` entirely (`audioDevice = 0`).
- Drop `SDL_RENDERER_PRESENTVSYNC` (lldb attach showed
  `SDL_RenderPresent()` genuinely blocked for many real seconds inside
  Metal's `nextDrawable` semaphore wait -- a real windowing/compositor
  quirk in this specific terminal-launched environment, stalling wall-
  clock pacing of the whole emulation loop).

Converted `~/Downloads/Apple_DOS_3.3_Master.dsk` and
`~/Downloads/Zork_I.dsk` to reinette's required flat 232960-byte
`.nib` format via `tools/build_nib_for_reference_emu.py` (written
earlier this session for the SDL2 native-build task).

Verification method: lldb `process attach` + direct RAM memory reads
of the running process (screenshots were unreliable in this
environment -- the window itself renders very slowly/incompletely due
to the same Metal drawable stall noted above, even though the
underlying CPU/disk emulation runs fine; memory reads bypass that
entirely and are ground truth).

## Real, verified results

**DOS 3.3 Master boot: SUCCESS.** Screen memory shows the complete,
correct real banner:
```
DOS VERSION 3.3           08/25/80
(LOADING INTEGER INTO LANGUAGE CARD)
APPLE II PLUS OR ROMCARD   SYSTEM MASTER
```
Matches exactly what was verified on our own RISC-V port with the real
alt-ROM last session (`docs/screenshot_dos33boot_altrom_asoft.png`,
`spike-reinette-port` branch's earlier boot-fix work). Settles cleanly
into an idle keyboard-wait state after boot.

**Zork_I.dsk boot: STUCK, but NOT crashed.** Screen memory stays at
just `"APPLE ]["` (the Monitor's own cold-start banner -- NOT any real
Zork boot text) for 45+ real seconds, while the process shows
sustained real CPU activity (~17%, steadily accumulating CPU time the
entire observation window -- confirmed via two separate `ps`+lldb
samples ~15s apart, memory content byte-identical between them despite
the ongoing CPU work). This is consistent with a real, ongoing
disk-retry loop that never successfully completes Zork's own boot
sector read.

## Why this matters for your investigation

This is an INDEPENDENT 6502 core (puce6502) + INDEPENDENT Disk][
nibble-read implementation (upstream reinette-II-plus, not derived
from or related to our `cpu6502.c`/`disk2_controller.c`) getting stuck
on the exact same real `Zork_I.dsk` image, never producing real game
text. If our own emulator's `nibble_shift()` timing model were the
whole story, I'd have expected THIS independent implementation (with
its own, differently-structured timing model -- see the earlier
apple2js cross-check session notes on reinette's real
call-parity/skip-toggle mechanism vs our elapsed-cycle model) to boot
this disk cleanly. It doesn't.

This shifts the weight of evidence toward: (a) something about this
SPECIFIC disk image/cracked variant needing exact real hardware timing
neither emulator nails, or (b) a genuinely hard timing-sensitive
protection/anti-piracy scheme in the real Zork I boot sector that trips
up naive/non-cycle-perfect nibble emulation generally, not specific to
our codebase's implementation choices. Doesn't rule out a real bug in
OUR emulator too, but it's no longer the only plausible explanation on
the table given this cross-check.

## Handoff

`/tmp/reinette_noaudio/` (diagnostic-patched build + source) and
`/tmp/reinette_nibs2/*.nib` (converted disk images) are still present
on this machine if you want to poke at them directly (lldb attach +
`memory read --format y --size 1 --count 960 --outfile <path> <addr>`
against `&ram + 0x400`, same as this investigation used -- get the
runtime address via `expr -f x -- (unsigned long)&ram + 0x400` after
attaching, since ASLR shifts it per-run). Both are scratch/disposable,
not committed anywhere -- happy to rebuild if you want them regenerated
after this session ends. Also messaged you directly via `maestri ask`
(async, may not have delivered yet) with the same summary.
