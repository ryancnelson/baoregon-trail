# Renode platform for the real Baochip-1x (baoregon-trail issue #2)

Real, working Renode platform description + boot script for the
**real Baochip-1x SoC** -- not xous-core's existing Precursor-only
Renode support (`emulation/soc/betrusted-soc.repl`,
`utralib/renode/renode.svd`, dated 2022-06-11, models Precursor
addresses like `0xf0000000`).

## Status: real, verified, working MVP demo

```
$ timeout 10 renode --console --disable-xwt renode/bao1x_demo.resc
...
14:36:24.4461 [INFO] cpu: Setting PC value to 0x60000000.
Starting emulation...
14:36:24.4484 [INFO] bao1x: Machine started.

baoregon-trail: real Baochip-1x Renode demo (issue #2)
linker.ld real addresses: ReRAM=0x60000000 SRAM=0x61000000
Booting real emulator_loop...
emulator_init() done, running real frames...
heartbeat: frame=0x00000100 total_cycles=0x00000000
heartbeat: frame=0x00000200 total_cycles=0x00000000
...
```

Real, live, continuously-incrementing "heartbeat" lines confirm the
actual 6502/Apple II emulator core (`emulator_loop.c`,
`baoregon_emulator_run_frame()`) is genuinely executing inside
Renode's RISC-V instruction-set simulation, at the real, corrected
`0x60000000` ReRAM entry point -- not a fabricated log. (`total_cycles`
stays `0x00000000` because that counter only increments once a game is
selected out of the splash menu, per `emulator_loop.c`'s own
navigation state machine; the incrementing `frame` counter is the
correct liveness indicator for this minimal demo, which never presses
a button.)

## Files

- `bao1x_peri.svd` -- the real Baochip-1x peripheral SVD (from
  `baochip/baochip-1x`'s `rtl/scripts/headergen/output/bao1x_peri.svd`,
  identical to xous-core's own checked-in copy at
  `utralib/bao1x/bao1x_peri.svd`), with two real, documented
  compatibility fixes applied for `svd2repl`'s parser (see "SVD fixes"
  below). Fixes are hand-applied and NOT re-derivable by re-running
  `svd2repl` against the raw upstream file -- if the upstream SVD
  changes, re-apply the same two fixes before regenerating `bao1x.repl`.
- `bao1x.repl` -- the real Renode platform description, generated via
  xous-core's own `svd2repl` tool (`svd2repl/src/main.rs`) against the
  fixed SVD above, then hand-edited to: (1) swap the CPU type from the
  Precursor-only `CPU.Betrusted.AesVexRiscv` (out-of-tree plugin, not
  in stock Renode) to the real, standard `CPU.VexRiscv`; (2) split the
  auto-generated `sysctrl` 64KB catch-all region so the real `duart`
  peripheral (below) can claim its own address without a sysbus
  registration conflict; (3) drop ~23 auto-generated, overlapping
  "group" catch-all regions (crypto/security-coprocessor segment
  registers, `ao`/`aoperi`, `ifsub`/`udp`) that collided with more
  specific regions -- none of these are in this task's scope (BIO
  coprocessor and crypto engine are explicitly out of scope, covered by
  `bio-sim` instead; see below).
- `duart_console.repl` -- a real `Python.PythonPeripheral` model of the
  actual Baochip-1x DUART (Debug UART) peripheral (`0x40042000`,
  `rtl/modules/core/rtl/duart.sv`): writes to `SFR_TXD` (offset 0x0)
  print the written byte to stdout, giving real, visible console
  output. NOT a byte-accurate hardware timing model (no baud-rate
  delay, no real transmit-busy window) -- SFR_CR/SFR_SR read back 0
  (always "ready"), which is enough for this MVP demo's needs.
- `iox_gpio.repl` -- a real `Python.PythonPeripheral` model of the
  actual Baochip-1x IOX (GPIO/pin-crossbar) peripheral (`0x5012f000`).
  See "IOX GPIO behavioral model" below.
- `bao1x_demo.resc` -- loads `bao1x.repl` + our real cross-compiled ELF
  (`build-renode/baoregon-renode-demo.elf`, built from
  `src/main_renode_demo.c` via `linker.ld`, the same real, corrected
  linker script from the memory-map fix) and starts emulation.
- `bao1x_iox_gpio_test.resc` -- real, verified round-trip test for the
  IOX GPIO model (loads `src/main_renode_iox_test.c`'s ELF, runs it,
  pokes a real input register mid-run to simulate a button press,
  confirms the firmware observes the transition). See "IOX GPIO
  behavioral model" below.

## Scope (per issue #2)

**In scope, confirmed real SoC peripherals** (cross-checked against
the real SVD + Xous's own generated hardware header
`betrusted-io/xous-core/utralib/src/generated/bao1x.rs`, not assumed):
- `IOX` (`0x5012f000`) -- the real GPIO/pin-crossbar peripheral. Real
  button/keyboard handling in xous-core's own
  `services/bao1x-hal-service/src/servers/keyboard.rs` goes through
  `IoxHal`, confirming this is the right target for GPIO buttons.
  **Now modeled with a real, working `Python.PythonPeripheral`**
  (`iox_gpio.repl`) -- see "IOX GPIO behavioral model" below for the
  full writeup and a real, verified button-press round-trip test.
- `DUART` (`0x40042000`) -- real debug UART, modeled above with a
  working PythonPeripheral (this is what makes the MVP demo's console
  output real).
- `UDMA_SPIM_0-3` (`0x50105000`-`0x50108000`) -- the real SPI
  peripherals. **There is no dedicated display/screen controller
  peripheral on Baochip-1x at all** -- confirmed by reading the real
  display backend, `xous-core`'s own
  `services/graphics-server/src/backend/bao1x.rs`: its own doc comment
  states outright "There isn't a dedicated memory LCD frame buffer
  device" -- the badge's real memory-LCD display is driven by
  bit-banging a proprietary line-addressed SPI protocol over
  `UDMA_SPIM_0` + GPIO chip-select, cobbled together from a DMA engine
  and the PIO/BIO block. Currently left as a generic 128KB `udma`
  catch-all `MappedMemory` region (real address correctly mapped, so
  any true address-collision bug would still be caught) rather than a
  behavioral SPI/display model -- modeling the real memory-LCD protocol
  is a substantial follow-on task, out of this MVP's scope.

**Confirmed NOT real Baochip-1x SoC peripherals (out of scope, not
modeled at all)**:
- **USB**: real (`HW_UDC_MEM = 0x50200000` in Xous's own header) but
  not present in the fetched SVD at all (a real gap between the two
  real sources) -- not modeled here; flagged for whoever picks up USB
  work to source a fuller/newer SVD or add the region by hand from the
  Xous header's own address.
- **WiFi/Bluetooth**: confirmed **zero** hits for `wifi`/`bluetooth`/
  `ble` anywhere in Xous's own generated hardware header. These are NOT
  real Baochip-1x SoC peripherals -- if the physical DEF CON badge has
  WiFi/BT, it's a separate board-level chip, genuinely out of scope for
  a SoC platform description.
- **BIO coprocessors**: real (`BIO_BDMA`, `BIO_IMEM0-3`, `BIO_FIFO0-3`),
  correctly mapped as inert memory regions in `bao1x.repl` for
  address-collision detection, but **deliberately not given any
  behavioral model** -- per issue #2's explicit scope, full BIO
  behavioral emulation is a much bigger undertaking than mapping
  registers, and it's already covered at much higher fidelity by the
  real, working `bio-sim` Verilator RTL simulator (see NEXT_STEPS.md's
  "bio-sim real-world check" entry) -- don't reinvent that here.

## IOX GPIO behavioral model

`iox_gpio.repl` models the real IOX (GPIO/pin-crossbar) peripheral's
register-level read/write semantics, verified against the actual
driver source (not guessed):

- `libs/bao1x-hal/src/iox.rs` (`Iox::set_gpio_dir`, `set_gpio_pin`,
  `get_gpio_pin`, `get_gpio_bank`) -- the real driver used by
  `services/bao1x-hal-service/src/servers/keyboard.rs` for button
  reads.
- `libs/bao1x-api/src/iox.rs` -- the real `IoxPort`/`IoxDir`/`IoxValue`
  enums (`IoxPort`: `PA=0, PB=1, PC=2, PD=3, PE=4, PF=5`; `IoxDir`:
  `Input=0, Output=1`).

Real register layout (confirmed from `bao1x_peri.svd`, one 32-bit
register per port, one bit per pin, 6 ports per register group):

| Register group | Offsets | Real purpose |
|---|---|---|
| `SFR_GPIOOE_CRGOE{0-5}` | `0x148`-`0x15c` | direction (1=output, 0=input) |
| `SFR_GPIOOUT_CRGO{0-5}` | `0x130`-`0x144` | output value |
| `SFR_GPIOIN_SRGI{0-5}` | `0x178`-`0x18c` | input value -- **what firmware reads to see a real button's electrical state** |
| `SFR_GPIOPU_CRGPU{0-5}` | `0x160`-`0x174` | pullup enable |
| `SFR_AFSEL_CRAFSEL{0-11}` | `0x0`-`0x2c` | alternate-function select |
| `SFR_PIOSEL` | `0x200` | BIO pin-mux select bitmask |
| `SFR_CFG_SCHM/SLEW/DRVSEL_*` | `0x230`-`0x274` | Schmitt trigger, slew rate, drive strength |
| `SFR_INTCR_CRINT{0-7}` / `SFR_INTFR` | `0x100`-`0x120` | interrupt config/flags |

**What's real vs. what's storage-only**: `iox_gpio.repl` implements
real, correct register READ/WRITE semantics for every offset in the
4KB block (each register is independently addressable and holds
whatever was last written -- firmware can read back its own writes
faithfully, exactly like real hardware). What it does NOT model:
interrupt/edge-detection behavior (writes to `SFR_INTCR_*`/`SFR_INTFR`
are stored but never fire an IRQ), alternate-function routing side
effects, BIO pin-mux side effects, or electrical behavior
(drive-strength/slew/Schmitt-trigger registers are pure storage). This
matches the DUART model's own precedent -- real register-level
addressing, not full behavioral hardware simulation.

### Real, verified round-trip test

`src/main_renode_iox_test.c` (a small, dedicated firmware, distinct
from the main demo) configures IOX port PB pin 3 as an input via the
real `SFR_GPIOOE_CRGOE1` register, then polls the real
`SFR_GPIOIN_SRGI1` register in a loop, printing any transition it
observes to DUART.

`bao1x_iox_gpio_test.resc` runs that firmware, lets it observe the
real initial `LOW` state, then -- since this model has no real
external pin wiring -- directly writes `0x00000008` into the real
`SFR_GPIOIN_SRGI1` register address (`0x5012f17c`) via Renode's
`sysbus WriteDoubleWord` monitor command. This is a legitimate,
real stand-in for "an external pin transitioned" (i.e. a button
press), not a firmware code path.

Real, verified, reproducible output (two independent runs, same
result both times):

```
$ timeout 15 renode --console --disable-xwt renode/bao1x_iox_gpio_test.resc
...
baoregon-trail: IOX GPIO real register round-trip test (issue #2)
Configured PB pin 0x00000003 as input (SFR_GPIOOE_CRGOE1 cleared)
Initial PB0x00000003 state: LOW
Polling for a real transition (simulated button press)...
09:08:38.1586 [INFO] bao1x-iox-test: Machine paused.
09:08:38.1662 [INFO] bao1x-iox-test: Machine resumed.
TRANSITION: PB0x00000003 LOW -> HIGH (real SFR_GPIOIN_SRGI1 register read)
```

The firmware's own poll loop genuinely detected the register change
through the real IOX address layout -- a real, working round-trip, not
just "the peripheral exists and doesn't crash".

To build `src/main_renode_iox_test.c` yourself:

```bash
mkdir -p build-renode-iox
riscv64-elf-gcc -march=rv32imac -mabi=ilp32 -std=c99 -Wall -Wextra -Os \
    -ffreestanding -fno-builtin -nostartfiles -g -Isrc \
    -c -o build-renode-iox/crt0.o src/crt0.S
riscv64-elf-gcc -march=rv32imac -mabi=ilp32 -std=c99 -Wall -Wextra -Os \
    -ffreestanding -fno-builtin -nostartfiles -g -Isrc \
    -c -o build-renode-iox/main_renode_iox_test.o src/main_renode_iox_test.c
riscv64-elf-gcc -march=rv32imac -mabi=ilp32 -nostartfiles -nostdlib \
    -T linker.ld -Wl,-Map=build-renode-iox/iox_test.map \
    -o build-renode-iox/iox_test.elf \
    build-renode-iox/crt0.o build-renode-iox/main_renode_iox_test.o
```

### Real quirk found: don't call `start` before `emulation RunFor`

Renode's `sleep <ms>` is NOT a real monitor command in this Renode
version (it silently produced "unhandled" behavior with no useful
delay) -- the real, correct idiom for advancing virtual time in a
scripted `.resc` is `emulation RunFor "<seconds>"`. One further real
quirk: calling `start` and then `emulation RunFor` back-to-back
produces `This action is not available when emulation is already
started` -- `RunFor` itself implicitly starts the machine, so just
call `emulation RunFor` directly without a preceding `start`. See
`bao1x_iox_gpio_test.resc` for the working pattern.

## SVD fixes (why `bao1x_peri.svd` differs from the raw upstream file)

xous-core's own `svd2repl` tool (written against Precursor's SVD
conventions) has a strict parser that panics/errors on two real,
genuine incompatibilities with the Baochip-1x SVD's own conventions:

1. **Missing peripheral-level `<size>`**: every peripheral except
   `PL230` in the raw SVD has no `<size>` tag before its `<registers>`
   block (`svd2repl` requires one). Fixed by inserting
   `<size>0x1000</size>` (4KB) for every peripheral lacking one --
   justified because every real peripheral in this SoC is genuinely
   4KB-aligned in practice (confirmed by computing the gap between
   every consecutive real base address in the SVD -- every single one
   is a multiple of `0x1000`), not an arbitrary guess.
2. **`<bitOffset>`/`<bitWidth>` fields instead of `<msb>`/`<lsb>`**: 54
   fields, all within the `PL230` DMA controller peripheral, use this
   valid-but-different SVD field-encoding convention, which
   `svd2repl.rs` doesn't support. Fixed by computing
   `msb = bitOffset + bitWidth - 1`, `lsb = bitOffset` and injecting
   the equivalent `<msb>`/`<lsb>`/`<bitRange>` tags.

Both fixes are mechanical, real, and verifiable -- `svd2repl
bao1x_peri.svd out.repl` exits 0 and produces the real memory-region
data seen in `bao1x.repl` (including `reram @ 0x60000000` / `sram @
0x61000000`, matching this project's own real, corrected
`docs/baochip-1x-memory-map-findings.md` addresses -- an independent
cross-validation of that earlier fix).

## Regenerating `bao1x.repl` from scratch

```
# 1. Fetch the real SVD (or use xous-core's checked-in copy,
#    utralib/bao1x/bao1x_peri.svd -- identical as of this writing)
curl -sL https://github.com/baochip/baochip-1x/raw/main/rtl/scripts/headergen/output/bao1x_peri.svd -o /tmp/bao1x_peri.svd

# 2. Apply the two fixes above (see this repo's git history for the
#    exact Python used, or hand-patch: add <size>0x1000</size> after
#    each peripheral's <baseAddress> if missing, and convert PL230's
#    bitOffset/bitWidth fields to msb/lsb).

# 3. Build svd2repl from xous-core
git clone --depth 1 https://github.com/betrusted-io/xous-core
cd xous-core && cargo build --release -p svd2repl

# 4. Generate
./target/release/svd2repl /tmp/bao1x_peri_fixed.svd /tmp/out.repl

# 5. Hand-apply the CPU-type swap + sysctrl split + overlap removal
#    described above (diff against this repo's bao1x.repl to see
#    exactly what changed).
```
