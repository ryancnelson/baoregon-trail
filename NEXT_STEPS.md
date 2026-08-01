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
