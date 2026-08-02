# Project Goals — Bao-Oregon-Trail

## 🎯 Primary Objective

Build a bare-metal, high-performance Apple II emulator for the **Baochip-1x** silicon (DEF CON 34 badge) that boots *The Oregon Trail* directly from non-volatile ReRAM in under 50 milliseconds with zero external hardware dependencies.

---

## 🏆 Key Success Criteria

1. **100% Self-Contained Execution**
   * Boot *The Oregon Trail* (.dsk image) directly from internal 4.0 MiB ReRAM without requiring an SD card reader or external PSRAM.
2. **Instant-On Boot Time**
   * Cold boot to Oregon Trail title screen in `< 50ms` from power-on.
3. **Flawless Display Rendering**
   * Hardware-accelerated Apple II Hi-Res (280x192) memory un-swizzling and color rendering handled by a 700 MHz PicoRV32 BIO coprocessor.
   * **TEXT mode with real character-ROM glyph rendering** (added 2026-08-02, Ryan's wishlist): currently TEXT mode renders as solid black -- no character-generator ROM decoder exists yet. Target: decode the real `342-0133-a.chr` (4096-byte) character-generator ROM and render Apple II text-mode screen memory ($0400-$07FF, real row-interleaved layout) into readable on-screen text. First validation target: DOS 3.3's boot banner ("DOS VERSION 3.3"), already proven correct at the memory level (Step 7) but currently invisible. This also directly unlocks Zork I (primarily text-based) as a second real title.
4. **Authentic Apple II Audio**
   * Low-latency reproduction of the 1-bit `$C030` speaker toggles (click sounds, wagon wheel creaks, river crossing effects) via BIO hardware PWM.
5. **Badge Input Support**
   * Direct mapping of physical badge D-pad / action buttons to Apple II keyboard sequences (`RETURN`, numbers `1-9`, `Y`/`N`, arrow keys).
6. **Multi-Game "Cartridge" Menu**
   * Utilize remaining ~3.8 MiB of ReRAM to store 10–25 classic Apple II titles selectable via a retro boot menu.

---

## ⛔ Non-Goals

* **Full Apple IIgs / 16-bit Emulation**: This project targets 8-bit Apple II / IIe hardware (MOS 6502).
* **Mac OS / 68k Support**: Handled separately in 68k emulator projects.
* **Full Operating System Dependency**: We will NOT depend on heavy OS kernels; execution will target the bare-metal Dabao SDK / minimal runtime for maximum speed and lowest memory overhead.

---

## 📅 Roadmap & Milestones

### Milestone 1: 6502 Core & Verification (CLI Sandbox)
* [ ] Integrate lightweight C 6502 core (`fake6502` or custom jump-table engine).
* [ ] Run Klaus Dormann's 6502 functional test suite to ensure 100% flag and opcode accuracy.

### Milestone 2: Memory & Disk Controller Hooks
* [ ] Implement Apple II 64 KB memory map ($0000-$FFFF) in SRAM.
* [ ] Implement Disk II soft-switches ($C0E0-$C0EF) to stream 140 KB `.dsk` sectors directly from ReRAM pointers.
* [ ] Boot DOS 3.3 / ProDOS to Applesoft BASIC prompt in host simulator.

### Milestone 3: Dabao SDK & Hardware Integration
* [ ] Write RISC-V linker script for Baochip-1x (Code/ReRAM @ `0x20000000`, Data/SRAM @ `0x40000000`).
* [ ] Cross-compile with `riscv32-unknown-elf-gcc` (`-march=rv32imac -mabi=ilp32`).
* [ ] Validate execution on Dabao evaluation board / hardware simulator.

### Milestone 4: BIO Coprocessor Acceleration
* [ ] Program **BIO Core 0** (PicoRV32 @ 700 MHz) for Apple II Hi-Res video memory decoding and NTSC color rendering to badge SPI display.
* [ ] Program **BIO Core 1** for `$C030` speaker PWM synthesis.
* [ ] Program **BIO Core 2** for button scanning & debouncing.

### Milestone 5: Polish & Game Loader
* [ ] Embed *The Oregon Trail* (1985 5.25" version) into ReRAM section.
* [ ] Add boot selector menu for additional embedded games (*Carmen Sandiego*, *Karateka*, *Lode Runner*).
* [ ] Power-saving tuning (utilizing RISC-V `WFI` instructions between 6502 ticks).
