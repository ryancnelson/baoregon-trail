# Brainstorming & Architecture Notes — Bao-Oregon-Trail

## 🧠 Architectural Ideas & Design Decisions

### 1. 6502 Emulator Core Selection
* **Option A: `fake6502` (Public Domain C)**
  * *Pros*: Single-file C, extremely portable, tiny footprint (~1,000 lines of code).
  * *Cons*: Requires minor bugfixes for non-documented opcodes and cycle timing.
* **Option B: Custom RISC-V Jump-Table 6502 Engine**
  * *Pros*: Tailored opcode dispatch table (`void (*opcodes[256])()`), minimal branch mispredictions on Vexriscv pipeline.
  * *Cons*: Takes slightly longer to write and verify.
* *Decision*: Start with `fake6502` for fast PoC, verified with Klaus Dormann's test suite, then optimize with a jump-table if needed.

---

### 2. Apple II Hi-Res Video Memory Un-Swizzling on BIO Cores

The Apple II Hi-Res graphics buffer lives at `$2000`–`$3FFF` (8,192 bytes) in 64K RAM.
However, line numbers do not map sequentially:
```
Row 0   : $2000
Row 1   : $2400
Row 2   : $2800
...
Row 8   : $2080
Row 9   : $2480
...
Row 16  : $2100
```

#### BIO Core Silicon Architecture (CONFIRMED from bio-sim `sw/build.zig` & `sw/include/bio.h`):
* **Hardware Core**: 4x PicoRV32 cores @ 700 MHz running **RV32IMC** (32 registers, `zmmul` hardware multiply, compressed instructions).
* **Register Mapping**:
  * `x0` – `x15`: Standard C general-purpose registers (GPRs) for local logic, stack, and function calls.
  * `x16` – `x31`: **Special-purpose hardware peripheral MMIO registers** hardwired directly to silicon peripherals (e.g. `x16` = `pop_fifo0`, `x26` = `set_gpio_mask`, event masks, cycle counter).
* **Compiler Configuration**: Target `-march=rv32imc -mabi=ilp32 -ffixed-x16 -ffixed-x17 ... -ffixed-x31`. The compiler excludes `x16`–`x31` from general temporary register allocation, leaving them available for direct high-speed assembly I/O instructions (`mv %0, x16`).

#### BIO Core 0 Pipeline
1. Main CPU updates raw byte at `$2000` + offset in SRAM.
2. BIO Core 0 (700 MHz PicoRV32) reads raw SRAM bytes via fast local interconnect.
3. Uses a precomputed 192-entry line lookup table:
   ```c
   const uint16_t hires_line_offsets[192] = {
       0x0000, 0x0400, 0x0800, 0x0C00, 0x1000, 0x1400, 0x1800, 0x1C00,
       0x0080, 0x0480, 0x0880, 0x0C80, 0x1080, 0x1480, 0x1880, 0x1C80,
       ...
   };
   ```
4. Expands 7-bit pixel groups into 280 horizontal pixels, applying Apple II green/purple or orange/blue artifacting rules.
5. Scales 280x192 to target badge display resolution (e.g. 320x240 RGB565) and pushes over SPI DMA.

---

### 3. Audio Offloading & `$C030` Speaker Toggling

* The Apple II toggles speaker state on **any access** (read or write) to `$C030`.
* In software:
  ```c
  uint8_t apple2_read(uint16_t addr) {
      if (addr == 0xC030) {
          // Send 1-word IPC signal to BIO Core 1
          BIO1_TRIGGER_PWM_TOGGLE();
          return 0;
      }
      ...
  }
  ```
* BIO Core 1 toggles the physical PWM hardware output pin on the Baochip, generating authentic 1-bit clicks without slowing down the 6502 CPU loop!

---

### 4. Disk II Floppy Emulation (Fast Sector Read)

Instead of emulating the full physical Disk II stepper motor and raw GCR nibble tracks, we use a **fast-hook soft-switch driver**:
* Apple II disk soft-switches live at `$C0E0`–`$C0EF`.
* When DOS 3.3 or ProDOS requests a sector read, the emulator traps the read call and copies the 256-byte sector directly from the ReRAM memory address where `OregonTrail.dsk` is stored:
  ```c
  const uint8_t *disk_image_reram = (const uint8_t *)0x20080000; // Pointer in ReRAM
  ```
* Sector reads execute in **under 10 RISC-V clock cycles**, making disk loading instantaneous!

---

### 5. Multi-Game "Cartridge" Selection Menu & Hardware Inputs

#### Hardware Input Interface:
* **Button Count**: **3 physical buttons** (CONFIRMED by Ryan).
* **Button Layout & Geometry**: **TBD / UNCONFIRMED** (exact board positioning and button shapes pending clearer photo).
* **Logical Mapping (Layout-Agnostic)**:
  * **Button 0**: `PREV` / Up / Left (Move menu selection / option 1).
  * **Button 1**: `NEXT` / Down / Right (Move menu selection / option 2).
  * **Button 2**: `SELECT` / Enter / Action (Confirm selection / action trigger).
* **In-Game 6502 Input Binding**: The 3 logical buttons map directly to standard Apple II keyboard matrix soft-switches (`$C000` / `$C010` strobe).

#### Multi-Game Cartridge Layout:
With 3.8 MiB of free ReRAM remaining, we can embed multiple `.dsk` files. Offsets corrected 2026-07-31
(see section 6C's ReRAM Driver Guidance correction); verified in `src/cartridge_layout.c` /
`tests/test_cartridge_layout.c` -- base address is `0x20280000` (2.5 MiB into ReRAM), not the earlier
`0x20080000` typo:

```
ReRAM Offset     Size      Game Title
0x20280000       140 KB    The Oregon Trail (1985)
0x202A3000       140 KB    Where in the World is Carmen Sandiego?
0x202C6000       140 KB    Karateka
0x202E9000       140 KB    Lode Runner
0x2030C000       140 KB    Prince of Persia (Disk 1)
0x2032F000       140 KB    Ultima IV
```

A custom retro boot splash screen allows using the 3 physical badge buttons (`PREV`, `NEXT`, `SELECT`) to pick a game, pointing the disk controller trap to the selected ReRAM offset and resetting the 6502!

---

### 6. Discord #c-side Community Hardware Findings & SDK Integration

#### A. Public Bare-Metal SDK References:
* **`armstrongsubero/dabao-sdk`**: Real hardware-tested bare-metal C SDK containing 17 drivers and 26 verified examples (GPIO, UART, SPI, I2C, PWM, ADC, Timers, BIO, DMA, ReRAM, AES, SHA, TRNG, WDT, RTC, QSPI).
* **`Cramiumlabs/daric-sdk`**: Crossbar's official peripheral SDK under ThreadX (referenced by bunnie).

#### B. PL230 MDMA Hardware Erratum & Workaround:
* **Erratum**: Config-register writes to the PL230 MDMA controller silently fail on physical Baochip-1x silicon.
* **Workaround**: Dedicate **BIO Core 3** to a 700 MHz memory-copy DMA engine.
* **BIO Coprocessor Cluster Map**:
  * **BIO Core 0**: Video un-swizzling & display DMA (`src/bio_display.c`).
  * **BIO Core 1**: Speaker PWM audio toggling (`src/bio_audio.S`).
  * **BIO Core 2**: ReRAM disk sector fast-trap DMA.
  * **BIO Core 3**: Dedicated 700 MHz fast memory-copy DMA engine.

#### C. ReRAM Driver Guidance:
* **Unaligned Access & Readback**: ReRAM drivers require unaligned access handling and readback verification after sector writes.
* **Safe Partition Boundary**: Keep usable ReRAM disk partition space around 2.5 MB (`0x60280000`) to guarantee ample headroom for `.text`/`.rodata` program code. (Corrected 2026-07-31: an earlier draft of this note wrote `0x20080000`, which is only 512 KB past ReRAM's origin, not 2.5 MB. 2.5 MiB = 0x280000 bytes, so the correct *offset* is ReRAM-origin + 0x280000 -- see `src/cartridge_layout.h` / `tests/test_cartridge_layout.c` for the verified constant and regression test. Base address corrected again 2026-08-06: real Baochip-1x silicon puts ReRAM at `0x60000000`, not `0x20000000` -- see `docs/baochip-1x-memory-map-findings.md` -- so the real partition boundary is `0x60280000`, not `0x20280000`; only the base changed, the 2.5 MiB offset math above is unaffected.)

#### D. Silicon Peripheral Notes (ADC & SDIO):
* **ADC**: Max input reference is 1.208V bandgap (requires 2.2k/1k voltage divider for 3.3V inputs; source impedance <= 3.2k). Enable ADC after analog block init + stabilization delay.
* **SDIO**: 3 hardware silicon errata (hardcoded 38-cycle response timeout, clock stops in idle, `CMD2` fails). Workaround is SPI-mode SD access at 12.4 MHz.

---

### 7. Authoritative Power Architecture & Future Optimization Notes (bunnie huang)

#### A. Hardware Power Measurements (Authoritative from bunnie huang):
* **Full On** (700 MHz CPU + OLED display + External RAM + TRNG): `~40 mA`
* **WFI Sleep** (Main CPU in `wfi`, SRAM state retained, interrupt wakeable): `~12–14 mA` (mostly SRAM leakage; undervolting the 0.7V core can reduce further).
* **Deep Sleep** (RTC-only, SRAM powered down): `< 1 mA`

#### B. Independent BIO Core Execution During Main CPU Sleep:
* **Hardware Fact**: BIO coprocessor cores execute **100% independently** while the main VexRiscv CPU is suspended in `wfi` sleep (bunnie demonstrated driving WS2812 LED patterns entirely from BIO while the main CPU slept).
* **Future Architectural Optimization**:
  * Between 60Hz 6502 frame ticks, the main VexRiscv CPU can enter `wfi` sleep to save ~25 mA.
  * BIO Core 0 continues display DMA refresh and BIO Core 1 handles audio PWM completely autonomously while the main CPU sleeps!

#### C. PL230 MDMA Confirmation:
* **bunnie huang confirmed**: The PL230 MDMA hardware block is a dead end (closed-source, difficult to configure, and bus-bandwidth limited). Using BIO Core 3 as a 700 MHz dedicated memcopy DMA engine offers equal-or-better throughput with 100% open-source C control.

---

### 8. System Firmware ROM Breakdown & Sourcing (Stella & MAME Breakdown)

#### A. 12 KB vs 16 KB System ROM Discrepancy Reconciliation:
* **Classic Apple II / II+**: The system ROM was **12 KB** (`$D000–$FFFF`), containing 10 KB of Applesoft BASIC and 2 KB of Autostart Monitor (F8) ROM.
* **Apple IIe Hardware**: In actual Apple IIe hardware, the main system firmware consists of **two 8 KB ROM chips** (`342-0134-a.64` and `342-0135-b.64`), totaling **16 KB** spanning `$C000–$FFFF`. This 16 KB image includes internal I/O firmware (`$C100–$C7FF`) and expansion ROM routines (`$C800–$CFFF`) in addition to the classic `$D000–$FFFF` BASIC/Monitor space.
* **Resolution**: High-level specs referencing "12 KB" refer specifically to the `$D000–$FFFF` region; actual Apple IIe target firmware requires the full 16 KB image.

#### B. MAME Apple IIe Hardware ROM Set Breakdown:
* **`342-0134-a.64`** (8,192 B — Main firmware Part A, `$C000–$DFFF`)
* **`342-0135-b.64`** (8,192 B — Main firmware Part B, `$E000–$FFFF`)
* **`342-0133-a.chr`** (4,096 B — Character Generator)
* **`342-0132-c.e12`** (2,048 B — Keyboard/Misc ROM)
* **`341-0027-a.p5`** (256 B — Disk II Controller Boot PROM / P5A)
* **`341-0028-a.rom`** (256 B — Disk II Controller Sequencer ROM)
* *(Optional)* `sc01a.bin` (512 B — Votrax speech card)

#### C. External Sourcing Notice (Woz & Duke Guidance):
* **Homelab Fleet Status**: Confirmed by Stella (storage specialist) — ZERO Apple II/IIe ROM files or DOS 3.3/ProDOS `.dsk` disk images exist anywhere on the fleet.
* **Action Required**: Ryan will need to externally source these specific ROM files and a DOS 3.3 System Master `.dsk` (available from archive.org's Apple II software archive) before full hardware-level Disk II boot PROM integration can be built.
* **Standalone Architecture**: Our custom sector-trap loader (`src/disk_trap.c`, `src/disk_sector_layout.c`) executes 100% standalone without needing proprietary ROM blobs embedded in Flash!

