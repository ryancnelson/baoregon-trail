# Reinette-II-Plus Port Spike — Findings

Side-spike only, branch `spike-reinette-port`. NOT a replacement for the
main custom 6502/Disk2 emulator on `main` — evaluating whether
reinette-II-plus (MIT) is viable as an alternate/reference implementation
on our bare-metal QEMU RISC-V + ramfb target.

**Status as of 2026-08-02 (later same session): REAL FIRST QEMU BOOT
ACHIEVED.** Compiles, links, boots under real QEMU (`qemu-system-riscv32
-M virt -device ramfb -display cocoa`), runs a full 50M-cycle budget
without crashing, and produces genuine readable screen output: real
Apple II+ Autostart ROM cold-start banner text "APPLE ][" verified via
BOTH a QEMU monitor `pmemsave` memory dump (decoded through the real
Apple II text-row-interleave table) AND a visual
`hs.window:snapshot()` screenshot
(`docs/screenshot_reinette_first_boot.png`). No disk image attached for
this first attempt (real Apple II+ with an empty Disk][ slot 6 falling
through to the Monitor prompt is itself a valid, informative outcome).
Screen content stable/unchanged across two samples taken seconds apart
-- consistent with real hardware sitting at a monitor/autostart-retry
state, not a crash.

## What's done

- Vendored `third_party/reinette-II-plus/` and `third_party/puce6502/`
  (both MIT-licensed, LICENSE + attribution preserved, `git log` shows the
  vendoring commit).
- `src/reinette/puce6502_riscv.c/.h` — puce6502 CPU core with the
  stdio.h-dependent debug tail (`dasm`/`printRegs`/`getPC`/
  `_FUNCTIONNAL_TESTS` harness) stripped. Compiles clean, freestanding,
  rv32imac/ilp32, no libc.
- `src/reinette/reinette_core.c/.h` — SDL2-free hardware core ported out
  of reinette's `main()`: memory map, softswitches, Language Card banking,
  Disk][ nibble/phase stepping, paddle state machine. Speaker output
  routed through an injectable callback (intended target:
  `bunnie_audio_trigger_toggle`). Disk image loading changed from
  `fopen`/`fread` to `reinette_disk_attach(drv, const uint8_t*, read_only)`
  to match this project's embedded-nib-header pattern
  (`dos33_nib_disk_data.h` style). Compiles clean (one harmless
  `-Wmissing-braces` warning).
- Standalone link of `puce6502_riscv.o` + `reinette_core.o`: first attempt
  failed with undefined references (`__floatdisf`, `__divsf3`, `__mulsf3`,
  `__addsf3`, `__gtsf2`/`__ltsf2`, `__umoddi3`) — paddle logic uses
  `float`, softswitch dispatch uses 64-bit `%`. **Fixed** by adding
  `-lgcc` to the link line.

## Real blocker found (confirmed, not guessed) — RESOLVED

`Makefile.riscv`'s `LDFLAGS` did not include `-lgcc`. **Fixed** (commit
`0906891`) by adding `-lgcc`, AND a second real ordering bug found while
getting reinette-qemu to actually link (commit `1401cae`): `-lgcc` must
come AFTER the object files that reference libgcc symbols on the link
command line, not before — a static archive only resolves symbols
requested by files already processed. Both the main Baochip-1x target
and the new `reinette-qemu` target's link lines now correctly place
`-lgcc` last.

## Done this session (all 4 documented next steps)

1. `-lgcc` added to `Makefile.riscv`'s `LDFLAGS`, with correct link
   ordering (see above).
2. `src/reinette/reinette_shim.c/.h` — glue layer:
   - Speaker: wraps `bunnie_audio_trigger_toggle()`'s state-pointer API
     in a zero-arg closure matching `reinette_set_speaker_callback()`'s
     signature.
   - Keyboard: `reinette_shim_uart_poll()`, mirrors
     `uart_keyboard_bridge.h`'s injectable-callback pattern but writes
     directly into `reinette_KBD` (reinette's own independent memory
     model), not `apple2_mem_inject_key()`.
   - Video: **real simplification found this session** — reinette's
     TEXT-mode memory layout ($0400-$07FF/$0800-$0BFF, same
     row-interleave) is IDENTICAL to this project's own `apple2_mem.c`
     layout (confirmed by reading `reinette_core.c`'s
     `readMem()`/`writeMem()` directly). This means
     `bio_display_render_frame_auto_text_aware()` and
     `text_apple2_render_frame()` (this project's own real,
     MAME-verified `342-0133-a.chr` character ROM renderer) can be
     reused DIRECTLY against `reinette_ram` via `readMem()` — **no
     need to re-embed reinette's own font-normal.bmp/font-reverse.bmp
     bitmaps** as originally planned.
3. `tools/gen_reinette_rom_headers.py` + `src/reinette/reinette_roms.h`
   — embeds the real `appleII+.rom` (12288 bytes, independently
   SHA1-verified against `mame -listroms apple2p` as a byte-for-byte
   concatenation of 341-0011/12/13/14/15 + 341-0020-00) and `diskII.rom`
   (256 bytes, real 341-0027-a.p5 Disk][ boot PROM) as C arrays.
4. `src/main_qemu_reinette.c` + `Makefile.riscv`'s new `reinette-qemu`
   target — first real QEMU boot attempt, see Status section above for
   the real, verified result (readable "APPLE ][" banner text, both
   memory-dump and screenshot confirmed).

## Known rough edges / next steps for whoever continues this

- No disk image attached yet — real `.nib`-format Apple II+ disk data
  (this project's own `.dsk` assets are the wrong format for reinette's
  `reinette_disk_attach()`, which expects a flat 232960-byte nibble
  image; see `tools/build_nib_for_reference_emu.py`, written earlier
  this session for a different purpose — converting for reinette's
  OWN native SDL2 build — but directly reusable here too since the
  format requirement is identical).
- `emu_trace_heartbeat()` calls in `main_qemu_reinette.c` currently
  pass all-zero placeholder register values (`0,0,0,0,0`) since
  `puce6502_riscv.h` has `getPC()`/`setPC()`/`printRegs()` commented
  out (stripped for the freestanding build, per the file's own header
  comment). A real PC/register trace would need either re-adding a
  minimal `getPC()` accessor or reading `puce6502_riscv.c`'s internal
  state directly — not done this session, first-boot verification
  relied on direct screen-memory inspection instead (which is a
  stronger signal than a PC trace anyway for "did it produce readable
  output").
- Paddle/joystick input, disk write-back, and audio (never actually
  exercised — no speaker-toggle activity expected from a
  no-disk/monitor-prompt boot) are wired but unverified under real
  QEMU execution.
- Screen content was stable/unchanged across two samples a few seconds
  apart during the interactive loop — consistent with a real Apple
  II+ sitting at a monitor/autostart-retry state with no disk, but
  worth a longer-duration observation or a real disk image to confirm
  it's not silently stuck rather than idling correctly.

## License note

Both `reinette-II-plus` and `puce6502` are MIT-licensed
(github.com/ArthurFerreira2/reinette-II-plus,
github.com/ArthurFerreira2/puce6502). LICENSE files vendored intact under
`third_party/`. No GPL exposure from this spike.
