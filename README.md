# Bao-Oregon-Trail (baoregon-trail)

> **The Oregon Trail** running natively on the **Baochip-1x** open-silicon RISC-V SoC for the DEF CON 34 badge.

---

## 🌲 Overview

`baoregon-trail` is a bare-metal Apple II / IIe emulator targeted specifically at the **Baochip-1x** (BAO1X2S4F-WA) System-on-Chip designed by Andrew "bunnie" Huang.

By leveraging the Baochip's **350 MHz Vexriscv RISC-V CPU**, **4x 700 MHz PicoRV32 BIO coprocessors**, **2.0 MiB SRAM**, and **4.0 MiB ReRAM**, this project runs classic Apple II software—starting with *The Oregon Trail* (1985)—**100% on-chip** with zero external SD cards, zero external RAM, and instant (<50ms) boot times.

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
|  |  - 8 KB Apple II Raw Video Buffer  |  - Apple IIe Autostart ROM (12 KB)     |  |
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
