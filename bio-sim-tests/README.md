# bio-sim-tests

Real-hardware-target tests for BIO Core firmware against
[baochip/bio-sim](https://github.com/baochip/bio-sim) -- a Verilator
simulation of the actual `bio_bdma` RTL (the real PicoRV32 BIO
coprocessor cluster + its APB/FIFO peripheral interface), not a host
mock.

This directory holds our test sources; the actual simulator lives in a
separate checkout of `baochip/bio-sim` (external repo, not vendored
here -- it has its own RTL/Verilator/Zig toolchain and update cadence).

## One-time setup

```bash
git clone https://github.com/baochip/bio-sim.git ../bio-sim   # or anywhere; set BIO_SIM_DIR
cd ../bio-sim

# Verilator (host build path; see bio-sim's own README for the
# container-based alternative if you'd rather not install Verilator
# locally):
brew install verilator lz4   # macOS; use your platform's package manager otherwise

# ziglang (BIO program cross-compiler) -- PEP 668 means a venv is
# needed on most systems:
python3 -m venv /tmp/biovenv
/tmp/biovenv/bin/pip install ziglang

# Build the simulator binary. NOTE: as of 2026-08-01 this repo's own
# Makefile doesn't pass lz4's Homebrew include/lib paths through to
# Verilator's FST trace writer, which fails to find lz4.h even when
# lz4 is installed -- build with verilator directly instead, passing
# -I/opt/homebrew/include and -L/opt/homebrew/lib -llz4:
CFLAGS_EXTRA="-std=c++17 -pthread -I$(pwd)/sim -I/opt/homebrew/include" \
LDFLAGS_EXTRA="-pthread -L/opt/homebrew/lib -llz4" \
verilator --cc --exe --build -j 0 --trace-fst --assert \
  --timescale-override 1ps/1ps --top-module bio_bdma_wrapper \
  +define+SIM +define+USE_OSS_BRIDGE \
  -Wno-fatal -Wno-BLKANDNBLK -Wno-WIDTH -Wno-COMBDLY -Wno-CASEINCOMPLETE \
  --no-timing -Wno-UNOPTFLAT -Wno-TIMESCALEMOD -Wno-STMTDLY \
  -f rtl.f sim/sim_main.cpp \
  --CFLAGS "-std=c++17 -pthread -I$(pwd)/sim -I/opt/homebrew/include" \
  --LDFLAGS "-pthread -L/opt/homebrew/lib -llz4" \
  -Mdir build -o bio_sim

# Sanity check:
./build/bio_sim configs/smoke.jsonc   # expect "[selftest] ... -> PASS"
```

## Running our tests

```bash
cd /path/to/baoregon-trail
BIO_SIM_DIR=/path/to/bio-sim ZIGLANG_PYTHON=/tmp/biovenv/bin/python3 \
  python3 tools/run_bio_display_palette_biosim.py
```

or, if `bio-sim` is checked out as a sibling of this repo
(`../bio-sim`) and `ziglang` is on the default `python3`'s path:

```bash
make test-biosim
```

## What's tested

`sw/bio_display_palette/main.c` -- a hand-port of
`src/bio_display.c`'s `bio_display_color_to_rgb565()` palette lookup
(already host-unit-tested in `tests/test_bio_display.c` and
cross-compiled clean for the main-CPU rv32imac target) to the REAL BIO
core hardware interface: pop a `hires_color_t` ordinal from FIFO0, push
the looked-up RGB565 value to FIFO1. `configs/bio_display_palette.jsonc`
drives all 6 color cases through bio-sim's `fifo_write`/`run`/
`fifo_read` commands; `tools/run_bio_display_palette_biosim.py` builds
it with ziglang, runs it under the real Verilator-simulated `bio_bdma`
RTL, and asserts the FIFO1 output matches `src/bio_display.c`'s table
exactly.

RED verified by deliberately breaking one palette entry (BLACK ->
0x1234, then WHITE -> 0x9999 on a second pass) and confirming the
script fails with a real RTL execution mismatch before GREEN.

## Sync note

The palette table in `sw/bio_display_palette/main.c` is a manual copy
of `src/bio_display.c`'s `g_color_to_rgb565[]` -- there's no shared
header between this repo and bio-sim's tree. If you change one, change
the other and re-run this test.
