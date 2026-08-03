# Reinette-II-Plus Port Spike — Findings

Side-spike only, branch `spike-reinette-port`. NOT a replacement for the
main custom 6502/Disk2 emulator on `main` — evaluating whether
reinette-II-plus (MIT) is viable as an alternate/reference implementation
on our bare-metal QEMU RISC-V + ramfb target.

**Status as of 2026-08-02: partial. Compiles standalone. Never run under
QEMU. Not integrated with our video/keyboard/audio. Do not treat as a
working boot.**

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

## Real blocker found (confirmed, not guessed)

`Makefile.riscv`'s `LDFLAGS` does not currently include `-lgcc`. None of
the existing emulator modules on `main` use floating point or 64-bit
division, so this need never surfaced there before. Confirmed present at
`/opt/homebrew/Cellar/riscv64-elf-gcc/16.1.0/.../rv32imac/ilp32/libgcc.a`.
This is a straightforward one-line Makefile fix, not a hard blocker — just
not yet applied to the real `Makefile.riscv`.

## Not yet done

1. `Makefile.riscv` itself not modified — need to add `-lgcc` to LDFLAGS.
2. No shim layer connecting `reinette_core.c` to:
   - `ramfb_display.c` (video) — needs a render function converting
     reinette's TEXT/LoRes/HiRes video state into
     `ramfb_present_frame565`-compatible pixels. TEXT mode needs reinette's
     font bitmaps (`third_party/reinette-II-plus/assets/font-*.bmp`)
     re-embedded as C arrays first, since there's no filesystem on target.
   - `uart_keyboard_bridge.c` (keyboard input).
3. No `main_qemu_reinette.c` entry point or corresponding `Makefile.riscv`
   target.
4. **Never run under QEMU at all** — only isolated host cross-compiles/
   links of the two new source files done so far, no full target binary
   built or booted.

## Immediate next steps for whoever continues this

1. Add `-lgcc` to `Makefile.riscv`'s `LDFLAGS`.
2. Write `reinette_shim.c`:
   - Wire `reinette_core`'s speaker callback to `bunnie_audio_trigger_toggle`.
   - Wire keyboard to `uart_keyboard_bridge` / direct latch writes.
   - Write a render function: reinette TEXT/LoRes/HiRes → ramfb pixels
     (needs embedded font data first for TEXT mode).
3. Add `main_qemu_reinette.c` + Makefile.riscv target.
4. First real QEMU boot attempt. Report exact QEMU/serial output — do not
   claim success without a captured screenshot or trace, per project norms
   (see `NEXT_STEPS.md`'s emu_trace verification standard).

## Effort estimate

Rough order of magnitude, not a commitment: getting to a first QEMU boot
attempt (garbled or blank screen acceptable, just needs to run) is
probably 1-3 more focused sessions — the hard parts (CPU core + hardware
core porting, freestanding compile) are done; what's left is glue code
(shim + font embedding + Makefile target), which is mechanical but not
zero-effort given the font/video format translation.

## License note

Both `reinette-II-plus` and `puce6502` are MIT-licensed
(github.com/ArthurFerreira2/reinette-II-plus,
github.com/ArthurFerreira2/puce6502). LICENSE files vendored intact under
`third_party/`. No GPL exposure from this spike.
