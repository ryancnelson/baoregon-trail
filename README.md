# Bao-Oregon-Trail (baoregon-trail)

> **How vibe-coded is this? VERY!** This is an AI-agent-assisted project
> (Claude/Hermes-driven crew, human in the loop) built by someone who is
> not a microcontroller/embedded-systems expert. Treat claims here with
> appropriate skepticism, expect rough edges, and don't assume any of
> this reflects professional embedded engineering practice.

> **Goal:** get **The Oregon Trail** running natively on the **Baochip-1x** open-silicon RISC-V SoC for the DEF CON 34 badge. We're not there yet — this is a work in progress.

---

## 🌲 Overview

`baoregon-trail` is a bare-metal Apple II / IIe emulator being built for
the **Baochip-1x** (BAO1X2S4F-WA) System-on-Chip designed by Andrew
"bunnie" Huang.

The plan is to leverage the Baochip's **350 MHz Vexriscv RISC-V CPU**,
**4x 700 MHz PicoRV32 BIO coprocessors**, **2.0 MiB SRAM**, and **4.0 MiB
ReRAM** to run classic Apple II software — starting with *The Oregon
Trail* (1985) — **100% on-chip**, with zero external SD cards, zero
external RAM, and instant (<50ms) boot times. Real Baochip-1x hardware
hasn't arrived yet, so none of this has been verified on the actual
target chip — see "Current Status" below for what's actually been tested
so far (host-native and QEMU only).

---

## 💡 Architecture & Tech Stack

```
+-----------------------------------------------------------------------------------+
|                                 BAOCHIP-1X SOC                                    |
|                                                                                   |
|  +--------------------------------+   +----------------------------------------+  |
|  |     VEXRISCV MAIN CPU          |   |        BIO COPROCESSOR CLUSTER         |  |
|  |   350 MHz (RV32-IMAC + MMU)    |   |     4x PicoRV32 @ 700 MHz (RV32-EMC)   |  |
|  | (Executes 6502 CPU & DOS 3.3)  |   |   (Handles Display, Audio & Input)     |  |
|  +--------------------------------+   +----------------------------------------+  |
|                  |                                        |                       |
|                  +--------------------+-------------------+                       |
|                                       |                                           |
|  +------------------------------------+----------------------------------------+  |
|  |  2.0 MiB Internal SRAM             |  4.0 MiB ReRAM (XIP Flash-like Storage)|  |
|  |  - 64 KB Emulated Apple II RAM     |  - Bootloader & Emulator Runtime (~32KB|  |
|  |  - 8 KB Apple II Raw Video Buffer  |  - Apple IIe System Firmware ROM (16 KB)  |  |
|  |  - 150 KB BIO RGB565 Framebuffer   |  - OregonTrail.dsk Floppy Image (140 KB|  |
|  |  - Emulator Stack & Variables      |  - 3.8 MB Free (25+ Extra Apple II Games)|  |
|  +------------------------------------+----------------------------------------+  |
+-----------------------------------------------------------------------------------+
```

---

## 📂 Project Navigation

* 🎯 **[PROJECT_GOALS.md](PROJECT_GOALS.md)** — Core objectives, target milestones, and non-goals.
* 🧠 **[BRAINSTORM.md](BRAINSTORM.md)** — Architectural ideas, video memory un-swizzling on BIO cores, audio hooks, badge input mapping.
* 🚀 **[NEXT_STEPS.md](NEXT_STEPS.md)** — Immediate actionable task list to start building.

---

## ⚡ Quick Specs

* **Emulated System**: Apple II / IIe (MOS 6502 @ 1.023 MHz, 64 KB RAM).
* **Display Output**: 280x192 Apple II Hi-Res graphics scaled & converted to badge display (320x240 / 480x320 RGB565 or monochrome).
* **Audio**: 1-bit Apple II `$C030` speaker toggles synthesized via BIO PWM pin.
* **Storage Requirement**: 0 External Pins (Fully embedded in ReRAM).

---

## 🚧 Current Status (work in progress, hardware not yet in hand)

Nothing here has run on real Baochip-1x silicon yet — hardware is
expected soon. What follows is what's actually been built and tested so
far, host-native and under QEMU only.

581 tests passing (host-native C test suite, `make test`), including a
full pass of the industry-standard **Klaus Dormann 6502 functional test
suite** against our own from-scratch CPU core.

**What's actually been tested so far:**
- 6502 CPU core: passes the Klaus Dormann reference suite (~30.6M
  simulated instructions) — a good sign, not a guarantee it's bug-free
- Apple II memory map + soft-switch dispatch: tested against real DOS 3.3
  boot sequences
- Hi-Res video decode + NTSC color-artifact rendering: tested, and
  exercised end-to-end by converting and rendering a real 1985 Apple II
  title screen (see screenshot below) — on the host, not on target
  hardware
- RISC-V cross-compile for the Baochip-1x target (rv32imac/ilp32): builds
  clean, correct memory placement (verified via
  `tools/check_linker_placement.py`) — untested on real hardware
- Runs as compiled RISC-V machine code under **QEMU's generic `virt`
  machine** (`-M virt`): confirmed via live CPU register inspection
  (non-zero PC/SP, not stuck at reset) and a byte-exact memory dump of
  the emulated Apple II screen after execution. QEMU's `virt` machine
  does not model Baochip-1x's actual peripherals (ReRAM, BIO
  coprocessors, display) — this only verifies the core emulator logic
  runs correctly as RISC-V code, not that it works on the real chip

**What's still ahead, and still unverified:** real Baochip-1x hardware
bring-up — BIO Core display-DMA firmware, real button/display wiring,
and confirming the memory map/timing assumptions above actually hold on
silicon. The software side is further along than the hardware
integration; treat everything above as "looks right in simulation," not
"works on the badge."

### Screenshot

The real Oregon Trail (1985, MECC) title screen, converted to Apple II
Hi-Res format and rendered through our decode pipeline, run host-side and
under QEMU — not yet on real Baochip-1x hardware:

![Oregon Trail title screen, rendered via our own Hi-Res pipeline](docs/screenshot_oregon_trail_title.png)

### Try it yourself

```bash
# Build + run the composed emulator (host-native, no cross-compile needed):
cc -std=c99 -Wall -Wextra -Isrc -DFB_TERMINAL_VIEWER_NO_MAIN \
  -o tools/bin/oregon_trail_runner tools/oregon_trail_runner.c tools/fb_terminal_viewer.c \
  src/apple2_mem.c src/cpu6502.c src/disk_sector_layout.c src/disk_trap.c \
  src/bunnie_audio.c src/video_apple2.c src/bio_display.c src/lores_apple2.c
python3 tools/convert_image_to_hires.py tools/assets/oregon_trail_title.png tools/oregon_trail_hires.bin
cd tools && ca65 oregon_trail_title.s -o oregon_trail_title.o && ld65 -C oregon_trail_title.cfg -o oregon_trail_title.bin oregon_trail_title.o && cd ..
./tools/bin/oregon_trail_runner tools/oregon_trail_title.bin tools/oregon_trail_title_data.bin
```

See `tools/README_oregon_trail.md` for the full pipeline writeup, including
a real bug found and fixed (source/destination memory overlap corrupting
the rendered image).

