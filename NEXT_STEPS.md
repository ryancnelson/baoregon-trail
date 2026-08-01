# Next Steps — Action Plan for Bao-Oregon-Trail

Here is the immediate step-by-step action plan to begin implementation:

---

## 🛠️ Step 1: Repository & Toolchain Setup
- [ ] Initialize Git repository in `~/devel/baoregon-trail/`.
- [ ] Add `.gitignore` for C/RISC-V build artifacts (`*.o`, `*.elf`, `*.bin`, `build/`).
- [ ] Verify RISC-V cross-compiler installation (`riscv32-unknown-elf-gcc` or `clang` with `-target riscv32`).

---

## 💻 Step 2: C 6502 Emulator Core Sandbox
- [ ] Create `src/cpu6502.c` and `src/cpu6502.h` (based on lightweight C 6502 core).
- [ ] Write a host build harness (`Makefile` targeting host OS) to run Klaus Dormann's 6502 test suite binary (`6502_functional_test.bin`).
- [ ] Verify that all opcodes and status flags (`N`, `V`, `B`, `D`, `I`, `Z`, `C`) pass 100%.

---

## 💾 Step 3: Disk II & ReRAM Memory Mapping
- [ ] Create `src/apple2_mem.c` to handle the 64 KB memory space and soft-switch traps ($C000–$C0FF).
- [ ] Add disk loader utility in `tools/embed_disk.py` that converts `.dsk` files into C header byte arrays (`const uint8_t oregon_trail_dsk[]`).
- [ ] Implement fast-sector read trap for DOS 3.3 / ProDOS to load sectors directly from `oregon_trail_dsk[]`.

---

## 🖥️ Step 4: Video Un-Swizzler & BIO Core Prototype
- [ ] Create `src/video_apple2.c` for Hi-Res 280x192 graphics decoding.
- [ ] Write host SDL2 or terminal viewer to verify rendering of Apple II screen buffers.
- [ ] Port video un-swizzling loop to BIO core C template (`bio_core0.c`).

---

## 🚀 Step 5: Dabao SDK / Baochip Target Build
- [ ] Integrate Dabao SDK headers and hardware initialization scripts.
- [ ] Write `linker.ld` partitioning 4.0 MiB ReRAM (`.text`, `.rodata`, `.dsk_images`) and 2.0 MiB SRAM (`.data`, `.bss`, `apple2_ram`).
- [ ] Compile initial ELF/BIN binary and test on Dabao evaluation hardware / simulator!

---

## 🖥️ Step 6: QEMU `ramfb` Live Display (dev-only, NOT a Baochip-1x deliverable)

**Context:** as of 2026-08-01, `src/main_qemu.c`/`linker-qemu.ld` can already
cross-compile and run the real 6502 emulator + Apple II memory/video
pipeline as genuine RISC-V machine code under `qemu-system-riscv32 -M
virt` (confirmed via live register inspection + a byte-exact framebuffer
memory dump). Right now that only produces a one-shot memory dump that
gets rendered to a file/terminal *after* the guest halts — there's no
live, continuously-updating screen to actually watch while it runs.

**Goal:** wire up QEMU's `ramfb` device (`-device ramfb`, System bus, no
PCI needed — confirmed available on `virt`) so a real QEMU window shows
the emulator's Apple II screen live, updating every frame, instead of
requiring a manual memory dump + host-side render step per frame.

**Explicitly NOT the same as the real hardware display path** — `ramfb`
only exists because QEMU's firmware/`fw_cfg` layer implements it; real
Baochip-1x silicon has no such thing. This is purely a development
convenience to make iterating on the Apple II emulation side tangible and
watchable, not a step toward the real SPI/display driver (that's real,
separate, hardware-dependent work that has to wait for actual
hardware/datasheet — see PROJECT_GOALS.md's BIO Core 0 display-DMA item).

**What IS reusable / carries forward to the real hardware path:** the
`uint16_t framebuffer[BIO_DISPLAY_WIDTH * BIO_DISPLAY_HEIGHT]` RGB565
array `bio_display_render_frame*()` already produces is the stable
contract every output consumer reads from (already proven by
`fb_terminal_viewer.c` reading it one way). `ramfb` is just one more
consumer of that same array. Structure the ramfb integration so the
"grab the current frame" logic is cleanly separated from "hand it to
ramfb specifically" -- when real hardware/SPI display specs are in hand
later, only the second half gets replaced with an SPI-consumer; the
framebuffer-production side (6502 core, memory map, video decode) and the
"loop + grab current frame" side should both carry over unchanged.

- [x] Add a `ramfb`-consumer function alongside `fb_terminal_viewer_print()`
      in a new `tools/ramfb_display.c` (or similar) -- same "read the
      framebuffer array" contract, different output target.
- [x] Convert RGB565 -> ramfb's expected pixel format (`DRM_FORMAT_XRGB8888` / `XR24`).
- [x] Write the `fw_cfg`-based setup/registration code (`tools/ramfb_display.c`)
      to register the framebuffer with QEMU's `ramfb` device at boot.
- [x] Switch the QEMU launch from headless (`-nographic`) to an actual
      display backend (`-display cocoa` on macOS).
- [x] Wire into a continuous refresh loop (`src/main_qemu.c`)
      so the screen visibly updates frame-to-frame.
- [ ] Verify end-to-end: boot `build-qemu/baoregon-qemu.elf` under QEMU
      with `-device ramfb -display cocoa`, confirm a real window opens
      showing the Oregon Trail title screen. **Un-checked as of
      2026-08-01** -- see status note below; a real bug was found and
      fixed in fw_cfg registration, but the display still isn't showing
      the image end-to-end.

**Status update (2026-08-01, this session):** found and fixed a real bug
in `tools/ramfb_display.c`'s `ramfb_find_selector()` via
systematic-debugging (built `tools/test_ramfb_fwcfg.c` as a deterministic
UART-output repro instead of guessing from screendumps): the first
fw_cfg data-register read immediately after ANY selector write returns
stale/zero data, corrupting the directory walk. Fixed by re-selecting
FILE_DIR before actually walking it (see the fix's own comment in
`ramfb_find_selector()` for details) -- confirmed via the repro test:
`RESULT=PASS`, correctly finds `etc/ramfb` at selector `0x25`. Confirmed
`have_ramfb=1` in the real `main_qemu.c` build after this fix (added a
temporary UART diagnostic line -- safe to leave in, real hardware has no
UART here either).

**Still unresolved, next task for whoever picks this up:** even with
`have_ramfb=1` and the render/update loop confirmed actively running (PC
sampled repeatedly inside both `bio_display_render_frame_auto_text_aware`
and `ramfb_display_update`), a QEMU monitor `screendump` still shows
QEMU's own "Guest has not initialized the display (yet)" placeholder,
not the actual Oregon Trail image. This means: fw_cfg registration
genuinely succeeds now (confirmed), but something between "registration
succeeded" and "QEMU's ramfb device actually treats the surface as
initialized/live" is still not connecting. Worth checking, in order:
(1) whether `screendump` itself has known quirks with `ramfb`-backed
displays specifically (vs. `-display cocoa`'s live window actually
working fine -- these could differ; a human should look at the real
`-display cocoa` window directly rather than trust only `screendump`),
(2) whether QEMU's ramfb device needs an explicit "now mark this dirty/
refresh" signal beyond just writing pixel bytes to the registered
address, (3) whether the RAMFBCfg struct's `stride`/`fourcc`/`flags`
values need rechecking against this specific QEMU version's
`hw/display/ramfb.c` source rather than assumed from general docs.

Once this works, iteration on the actual Apple II side (getting DOS 3.3's
disk-boot chain further, keyboard input for the boot-splash menu, etc.)
becomes something that can be watched happening live, not re-derived from
memory dumps each time.

---

## 💾 Step 7: Real Disk II Controller Emulation (port from apple2js, MIT-licensed)

**Context:** tonight's session confirmed a real, hard blocker for booting
unmodified real Apple II disk images (Zork I, Apple DOS 3.3 System
Master): their boot code executes `JMP ($003E)`, an indirect jump into
the Disk II peripheral card's own ROM (e.g. `$C65C` for slot 6), expecting
that ROM to keep reading raw magnetic nibbles off the disk at a low
level. `src/disk_trap.c` deliberately doesn't implement this — it's a
fast-sector-read shortcut (given track/sector, copy 256 bytes straight
from ReRAM), not real Disk II hardware. This means real, unmodified
Apple II disk images cannot boot through our current pipeline; only
our own hand-assembled bootloaders (which call the fast-sector trap
directly) work.

**Decision (2026-08-01):** rather than write the real GCR/nibble-encoding
protocol from scratch, or port from AppleWin/MAME (both GPLv2 -- would
require GPL-licensing at least that module, incompatible with this
project's MIT license), port from
**[whscullin/apple2js](https://github.com/whscullin/apple2js)**, an
actively-maintained, **MIT-licensed** Apple II emulator (in TypeScript)
with a real, working Disk II implementation:

- `js/cards/disk2.ts` (851 lines) -- the Disk II card itself: phase-based
  stepper motor emulation (`PHASE0ON`/`PHASE0OFF`/etc. softswitches at
  `$C0Ex`), Q6/Q7 read/write-mode softswitches, boot ROM constants
  (`BOOTSTRAP_ROM_13`/`BOOTSTRAP_ROM_16`).
- `js/cards/drivers/NibbleDiskDriver.ts` (107 lines) -- the actual
  nibble-level read/write logic: `onQ6Low()` reads the next raw byte off
  the current track at the drive's head position and latches it, exactly
  the operation real disk boot code depends on.
- Also has WOZ-format disk support (`WozDiskDriver.ts`) for
  higher-fidelity images, if useful later.

**MIT attribution requirement:** when porting, must retain the MIT
license notice and attribute apple2js per its license terms -- this is a
real legal requirement of MIT, not optional. Add a comment header in the
ported C file(s) crediting `whscullin/apple2js` and including/pointing at
its MIT license text.

**Task breakdown:**
- [ ] Port `disk2.ts`'s softswitch dispatch ($C0E0-$C0EF phase/motor
      controls, Q6/Q7 mode switches) into a new `src/disk2_controller.c`
      -- this replaces/extends `disk_trap.c`'s scope, doesn't necessarily
      delete it (the fast-sector trap may still be useful for our own
      synthetic bootloaders/game-select menu).
- [ ] Port `NibbleDiskDriver.ts`'s track/head-position nibble read logic
      -- needs a real GCR-encoded/nibble-format disk image, not our
      existing flat 143,360-byte DOS-order `.dsk` format (that format is
      *sector* data, not raw nibble/flux data -- converting real DOS
      3.3/.dsk images to nibble format, or sourcing pre-nibblized `.nib`
      images, is a real sub-task here).
- [ ] Embed the real Disk II boot ROM (`341-0027-a.p5` +
      `341-0028-a.rom`, already sourced in `roms/` from tonight's
      session) at the address the boot code actually jumps to (`$Cn00`
      range, slot-dependent) so `JMP ($003E)` lands somewhere real.
- [ ] Verify against a real target: boot `Apple_DOS_3.3_Master.dsk`
      (or Zork_I.dsk) through the composed system and confirm it reaches
      the same real DOS 3.3 banner/prompt state already confirmed via
      MAME earlier tonight (`tools/fixtures/mame-captures/` has the
      verified reference RAM dump to compare against).
- [ ] Once working, revisit whether `ramfb`/QEMU live display (Step 6)
      should show this real-disk-boot path instead of (or alongside) the
      synthetic bootloader demos.
