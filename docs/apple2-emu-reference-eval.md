# Apple II emulator reference eval — non-GPL candidates (2026-08-02)

Purpose: shortlist + hands-on eval of permissively-licensed Apple II emulator
projects worth mining for architecture/code when porting to the QEMU RISC-V
rig / bare-metal target. NOT a wholesale-adopt decision — this project already
has its own 6502 core, memory map, and Disk II controller (ported from
apple2js per NEXT_STEPS.md Step 7). This is reference material for gaps
(video pipeline, disk timing details, general architecture patterns).

Clones live at `~/devel/apple2-emu-refs/` (sibling dir, NOT inside this repo
— avoids vendoring/lock conflicts with the active file-locks in
`.file-locks/`). Not committed to this repo; pure reference checkout.

## Hard requirement: license

MIT / BSD / Apache-2.0 / zlib / public domain only. GPL (any version)
excluded — confirmed via GitHub API `license` field per repo, not README
claims.

## Shortlist (verified license + hands-on)

### 1. reinette-II-plus — MIT — `ArthurFerreira2/reinette-II-plus`
- License confirmed via GitHub API: MIT.
- Two source files only: `reinetteII+.c` (~910 lines, machine+SDL2 glue) +
  `puce6502.c`/`.h` (CPU core, also MIT, separate repo `puce6502`).
- **Built clean on macOS host** (exit 0): `gcc "reinetteII+.c" puce6502.c
  -std=c11 -Wall -O3 -I/opt/homebrew/include -L/opt/homebrew/lib -lSDL2 -o
  reinetteII_host` — only warnings, no errors, produced working binary.
- Memory map is dead simple and directly comparable to ours: `ram[0xC000]`,
  `rom[0x3000]` at `$D000`, language card banks, disk][ prom at `$C600`.
  Soft-switch dispatch is a single function keyed on `(address & 0xF000) ==
  0xC000`.
- SDL2 usage is isolated to `main()`: window/renderer/audio init + event
  loop. Video output is a straightforward SDL_Renderer blit — would need a
  full rip-and-replace for a custom framebuffer, not a clean seam, but the
  surface area is small (single file, ~910 lines total).
- Verdict: **best size/completeness tradeoff**. Good candidate to read line
  by line for soft-switch/language-card edge cases our own emulator might be
  missing. Full SDL2 replacement is a moderate afternoon's work given the
  file is this compact.

### 2. pico-iie — MIT — `pyrex8/pico-iie`
- License confirmed via GitHub API: MIT. Depends on `floooh/chips` (6502
  core) — zlib license, also permissive.
- **Purpose-built for bare-metal (Pi Pico, no OS)** — closest architectural
  analog to our target of any candidate found.
- Confirmed real platform/core decoupling by inspection:
  - `common/video.c` — platform-agnostic: produces VGA-style scanline
    color-index data (`hcolor[]` palette, `VIDEO_RESOLUTION_X/Y` = 280x192,
    same native Apple II resolution we use) with zero Pico-SDK includes.
  - `mcu/vga.c` — Pico-specific PIO/DMA output layer, isolated from the
    above.
  - `main.c` pulls in Pico SDK headers directly (`pico/stdlib.h`,
    `hardware/pio.h`, `hardware/dma.h`, etc.) — the seam is clean at the
    `common/` vs `mcu/` directory boundary, exactly matching how you'd want
    to split "core emulation" from "target framebuffer driver."
- Verdict: **best architectural reference for the framebuffer swap**. Don't
  need to port the Pico-specific PIO/DMA code (irrelevant to RISC-V/QEMU) —
  but `common/video.c`'s scanline-generation logic (color-index buffer per
  scanline, matching 280x192 native res) is a solid template for how to
  drive our own custom framebuffer without carrying any SDL/host-OS
  dependency.

### 3. puce6502 — MIT — `ArthurFerreira2/puce6502` (component, not full emulator)
- Standalone MOS 6502 core backing reinette-II-plus. Small (168K checkout).
  Worth a diff-check against our own 6502 core if any opcode/flag edge cases
  come up — independent implementation, same permissive license, easy to
  read.

### Noted but not pursued further
- **bobbin** (MIT, `micahcowan/bobbin`) — actively maintained, but no
  graphics/sound emulation (terminal/curses-oriented only) — not useful for
  the framebuffer-porting goal specifically. Could be worth a look later for
  disk-format handling (.dsk/.do/.po/.nib/.hdv) if disk format coverage gaps
  show up.
- **Applerm-II** (BSD-3-Clause, `toyoshim/Applerm-II`) — bare-metal ARM
  Cortex-M0 precedent, abandoned since 2014, ARM assembly (not portable to
  RISC-V without a rewrite). Architectural precedent only, not a code
  source.
- **TomHarte/CLK** (MIT) — well-maintained multi-system emulator including
  Apple II, but 421MB repo / heavy C++ abstraction across dozens of
  systems — too large relative to what we need.

### Explicitly excluded (GPL, confirmed via API, not adopted)
AppleWin (GPL-2.0), Epple-II (GPL-3.0), AppleWin-rs (GPL-2.0, inherits
AppleWin), apple2js is actually **MIT** (already in use per NEXT_STEPS.md
Step 7 — do not confuse with the excluded GPL group above).

## Next step (not yet done)
Read `common/video.c` in pico-iie fully against our own
`src/video_apple2.c` (or equivalent) to see if there's a cleaner scanline
color-index abstraction worth adopting for the QEMU `ramfb` / custom
framebuffer path. Did not touch any files under active `.file-locks/` edit
(`src/disk2_controller.c`/`.h` — Duke+Woz's Zork I nibble_shift() work) —
this eval is purely additive reference material in
`~/devel/apple2-emu-refs/`, outside this repo.
