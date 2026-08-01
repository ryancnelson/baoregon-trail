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
With 3.8 MiB of free ReRAM remaining, we can embed multiple `.dsk` files:

```
ReRAM Offset     Size      Game Title
0x20080000       140 KB    The Oregon Trail (1985)
0x200A3000       140 KB    Where in the World is Carmen Sandiego?
0x200C6000       140 KB    Karateka
0x200E9000       140 KB    Lode Runner
0x2010C000       140 KB    Prince of Persia (Disk 1)
0x2012F000       140 KB    Ultima IV
```

A custom retro boot splash screen allows using the 3 physical badge buttons (`PREV`, `NEXT`, `SELECT`) to pick a game, pointing the disk controller trap to the selected ReRAM offset and resetting the 6502!
