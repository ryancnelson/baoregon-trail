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

- [ ] Add a `ramfb`-consumer function alongside `fb_terminal_viewer_print()`
      in a new `tools/ramfb_display.c` (or similar) -- same "read the
      framebuffer array" contract, different output target.
- [ ] Convert RGB565 -> ramfb's expected pixel format (likely `xrgb8888`;
      confirm against QEMU's `ramfb` docs/source, don't assume).
- [ ] Write the `fw_cfg`-based setup/registration code (`main_qemu.c` or a
      new QEMU-specific module) to register the framebuffer with QEMU's
      `ramfb` device at boot.
- [ ] Switch the QEMU launch from headless (`-nographic`) to an actual
      display backend (`-display cocoa` on macOS; confirm what's
      available/works before assuming).
- [ ] Wire into a continuous refresh loop (not the current one-shot
      dump-then-halt pattern) so the screen visibly updates frame-to-frame.
- [ ] Verify end-to-end: boot `build-qemu/baoregon-qemu.elf` under QEMU
      with `-device ramfb -display cocoa`, confirm a real window opens
      showing the Oregon Trail title screen (or whatever's currently
      loaded), and that it's a live, refreshing display -- not a static
      single frame.

Once this works, iteration on the actual Apple II side (getting DOS 3.3's
disk-boot chain further, keyboard input for the boot-splash menu, etc.)
becomes something that can be watched happening live, not re-derived from
memory dumps each time.
