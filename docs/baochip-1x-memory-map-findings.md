# Baochip-1x Real Memory Map vs. Our Assumptions — Findings (2026-08-06)

**Status: real, verified, hardware-prep finding. Not yet acted on in
source (`linker.ld`/`cartridge_layout.h`/`rram_driver.h` still use the
OLD, WRONG base addresses as of this writing) -- this doc is the
record of what's wrong and exactly what needs to change, so whoever
picks up the actual fix (real base-address migration, ideally its own
dedicated PR with a full `make test` + RISC-V rebuild pass) has a
precise, sourced spec to work from.**

## Real, authoritative sources checked

1. `github.com/baochip/baochip-1x` (real SoC source), specifically:
   - `verilate/bao_common.py` — the real, general LiteX SoC target used
     for Verilator CI simulation (shared by `bao_soc.py`, `bao_udma.py`,
     `bao_sce.py` — the actual chip's simulation model, NOT the
     `arty/` directory's separate Xilinx Arty FPGA dev-board target,
     which has its own different, smaller `SRAM_SIZE=128*1024` and is
     a distinct prototyping platform, not the real Baochip-1x memory
     map).
   - `rtl/scripts/headergen/output/bao1x_peri.svd` (fetched live from
     `github.com/baochip/baochip-1x/raw/main/...` — not checked into
     the repo, generated on demand per `docs/src/ch00-00-rtl-overview.md`).
   - `VexRiscv/GenCramSoC.scala` (`mtVecInit = 0x60000000`).
2. `ci.betrusted.io/bao1x-cpu/cpu.html` — the real, live-hosted CPU
   core-complex bus documentation.
3. `github.com/betrusted-io/xous-core/blob/main/utralib/src/generated/bao1x.rs`
   — the real Xous OS's own generated hardware register header,
   auto-derived from the same RTL/SVD source above. Independently
   cross-checked against (1) and (2) -- all three sources agree
   exactly.

## The core finding: our ReRAM and SRAM base addresses are wrong

| Region | Our assumption (`linker.ld` today) | Real chip (3 independent sources agree) |
|---|---|---|
| ReRAM | `0x20000000`, 4 MiB | **`0x60000000`**, 4 MiB |
| SRAM  | `0x40000000`, 2 MiB | **`0x61000000`**, 2 MiB |
| XIP (extra flash-like storage) | *(not modeled at all)* | `0x70000000`, up to 128 MiB |

**Sizes match exactly** (4 MiB ReRAM, 2 MiB SRAM) -- only the base
addresses are wrong. This is good news: our internal layout reasoning
(64 KB Apple II RAM + 8 KB video buffer + 150 KB BIO framebuffer +
stack, all fitting inside 2 MiB SRAM; ~32 KB bootloader + 16 KB Apple
IIe ROM + game .dsk images fitting inside 4 MiB ReRAM) needs zero
redesign -- it's a straightforward base-address relocation, not an
architecture rethink.

**This is a real, serious conflict, not just a wrong number**: our
assumed SRAM base (`0x40000000`) is NOT free/unmapped space on real
silicon -- it's the START of the real AXI-lite peripheral bus
(`0x40000000-0x5FFFFFFF` per `ci.betrusted.io/bao1x-cpu/cpu.html`,
confirmed uncached). The very first real peripheral at that exact
address is the **ReRAM Controller (RRC)** itself
(`HW_RRC_MEM = 0x40000000` in the Xous header;
`rtl/modules/rrc/rtl/rrc.sv`, CrossBar Inc.'s real ReRAM IP control/
status registers) -- meaning our current linker script would place
the emulator's entire 64 KB Apple II RAM + framebuffers directly on
top of live ReRAM-controller hardware registers on real silicon. This
would not have caused any symptom in QEMU or host-native testing
(neither models real peripheral MMIO at that address), so it's exactly
the kind of bug that stays invisible until real hardware bring-up --
confirming this was worth catching now.

## Full real peripheral map (AXI-lite bus, 0x40000000-0x50200000+ range)

Extracted from the live SVD + Xous header (56 real peripherals total).
The ones most relevant to this project's plans:

| Peripheral | Base | Relevance |
|---|---|---|
| `RRC` (ReRAM Controller) | `0x40000000` | Real hardware conflict, see above |
| `CORESUB_SRAMTRM` | `0x40014000` | Real SRAM timing/trim control (not our emulated RAM -- a real trim/config register for the physical SRAM macro) |
| `SYSCTRL` | `0x40040000` | System control/reset |
| `DUART` | `0x40042000` | Debug UART (likely our `emu_trace.c`/UART console equivalent on real hw) |
| `TIMER_INTF` | `0x40043000` | Real hardware timer -- we currently derive all 6502 timing from software cycle-counting (`clockticks6502`); real hw timer could replace/verify this |
| `EVC` | `0x40044000` | Event controller (NMI/interrupt routing) |
| `BIO_BDMA` | `0x50124000` | **BIO coprocessor cluster control** -- directly relevant to `bunnie_audio.c`/`bio_display.c` plans |
| `BIO_IMEM0-3` | `0x50125000`-`0x50128000` | Per-BIO-core instruction memory (4 cores) -- where compiled BIO machine code (see the `bio-sim` check from this project's own earlier session) actually loads on real hardware |
| `BIO_FIFO0-3` | `0x50129000`-`0x5012C000` | Per-BIO-core FIFO registers -- the host-CPU <-> BIO-core communication channel (matches `bio-sim`'s own `sw/README.md` register-map documentation: x16-x19 FIFO registers) |
| `IOX` | `0x5012F000` | GPIO/pin multiplexer -- needed for real display/audio/input pin assignment |
| `PWM` | `0x50120000` | Hardware PWM -- our `bunnie_audio.c` currently does 1-bit speaker toggle in software; real hw PWM could be a genuinely better implementation path |
| `SDDC` | `0x50121000` | Likely display-controller-adjacent (SD/DisplayPort-like interface -- name suggests "Serial/SDDC", needs real datasheet confirmation, not yet investigated in depth) |
| `UDMA_SPIM_0-3`, `UDMA_I2C_0-3`, `UDMA_I2S` | `0x50105000`+ | Real SPI/I2C/I2S peripherals -- if the badge's actual display/audio hardware uses any of these (vs. raw BIO-core bit-banging), this is where their registers live |
| `UDMA_ADC` | `0x50114000` | Analog input (paddle/potentiometer equivalent? not yet investigated) |

**Not yet investigated in this pass** (flagging honestly rather than
guessing): exact pin-to-peripheral routing for the actual DEF CON 34
badge's specific display/audio/button hardware (this needs the
badge's own schematic/pinout docs, not just the SoC's generic
peripheral list -- the SoC has many more peripherals than any single
badge design will use). The `IOX` (IO crossbar/mux) peripheral is
almost certainly the right starting point once badge-specific pin
assignments are available.

## Files in this repo that encode the wrong base addresses

- `linker.ld` -- `RERAM ORIGIN = 0x20000000`, `SRAM ORIGIN = 0x40000000`
  (both wrong; sizes correct)
- `src/cartridge_layout.h` -- `CARTRIDGE_RERAM_ORIGIN 0x20000000u`
  (wrong; the *offset* within ReRAM, `0x280000`/2.5 MiB, stays correct
  once the base is fixed)
- `src/rram_driver.h` -- doc comment references `0x20280000` (same
  fix needed)
- `linker-qemu.ld` -- header comment (lines 4-5) references the old
  wrong addresses when explaining why it diverges from the real
  target; QEMU's own `virt` machine RAM base (`0x80000000`) is
  correct and unaffected (confirmed via `qemu-system-riscv32 -machine
  dumpdtb` per this file's own existing comment) -- only the
  *comment's* description of the real target is stale, not any actual
  behavior.

## Suggested next step (not done in this pass -- scoping only)

A real base-address migration (`0x20000000`->`0x60000000` for ReRAM,
`0x40000000`->`0x61000000` for SRAM) across `linker.ld`,
`cartridge_layout.h`, `rram_driver.h`, and any doc/comment references,
followed by a full `make test` + `make -f Makefile.riscv riscv-check`
pass to confirm nothing else depends on the old addresses. This is a
mechanical, low-risk change (matches sizes exactly, just shifts base)
but should get its own dedicated pass with careful `grep`-driven
verification that every reference was caught, rather than being
folded into unrelated work.

## Also verified real (no conflict, no action needed)

- Reset vector: real chip resets into ReRAM XIP execution at `0x60000000
  + boot_offset` (`VexRiscv/GenCramSoC.scala`'s `mtVecInit`,
  `verilate/bao_common.py`'s `trimming_reset` signal) -- matches our
  own architecture's core assumption ("code executes directly in
  place from ReRAM, no copy-to-RAM step") exactly, just at the
  corrected base.
- CPU ISA: real chip is RV32-IMAC-Zkn(e/d) (adds AES crypto
  extensions on top of IMAC) via a VexRiscv core -- our own
  `-march=rv32imac -mabi=ilp32` target flags remain a valid, safe
  subset; we don't currently use the Zkn extensions and don't need to.
- 4x PicoRV32 BIO coprocessors @ 700 MHz -- confirmed for real via
  `bio-sim`'s own RTL (`rtl/picorv32.v`) in the earlier session's
  hands-on check; this session's SVD/Xous cross-check confirms the
  BIO's real bus addresses (`BIO_BDMA`/`BIO_IMEM*`/`BIO_FIFO*` above)
  line up with what `bio-sim`'s own register-map doc
  (`sw/README.md`) already described in x16-x31 CPU-register terms
  (the FIFO/GPIO/event register conventions), giving two independently-
  sourced confirmations of the same real BIO programming model.
