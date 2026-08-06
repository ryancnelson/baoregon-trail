#!/bin/bash
# renode/build_and_run_demo.sh -- build src/main_renode_demo.c against
# the REAL linker.ld (ReRAM @ 0x60000000, SRAM @ 0x61000000) and run it
# under Renode using renode/bao1x.repl + renode/bao1x_demo.resc.
#
# Usage (from the repo root):
#   bash renode/build_and_run_demo.sh            # build + run interactively
#   bash renode/build_and_run_demo.sh --build-only   # just build the ELF

set -euo pipefail
cd "$(dirname "$0")/.."

RISCV_CC="${RISCV_CC:-riscv64-elf-gcc}"
BUILD_DIR="build-renode"
mkdir -p "$BUILD_DIR"

SRCS="main_renode_demo cpu6502 apple2_mem disk2_controller disk_sector_layout disk_trap bunnie_audio video_apple2 lores_apple2 text_apple2 bio_display cartridge_layout rram_driver boot_splash emulator_loop boot_perf"

CFLAGS="-march=rv32imac -mabi=ilp32 -std=c99 -Wall -Wextra -Os -ffreestanding -fno-builtin -nostartfiles -g -Isrc"

echo "--- Assembling crt0.S ---"
"$RISCV_CC" $CFLAGS -c -o "$BUILD_DIR/crt0.o" src/crt0.S

echo "--- Compiling sources ---"
OBJS="$BUILD_DIR/crt0.o"
for f in $SRCS; do
    "$RISCV_CC" $CFLAGS -c -o "$BUILD_DIR/$f.o" "src/$f.c"
    OBJS="$OBJS $BUILD_DIR/$f.o"
done

echo "--- Linking (real Baochip-1x linker.ld: ReRAM=0x60000000 SRAM=0x61000000) ---"
"$RISCV_CC" -march=rv32imac -mabi=ilp32 -nostartfiles -nostdlib \
    -T linker.ld -Wl,-Map="$BUILD_DIR/baoregon-renode-demo.map" \
    -o "$BUILD_DIR/baoregon-renode-demo.elf" $OBJS

echo "--- Verifying section placement ---"
python3 tools/check_linker_placement.py "$BUILD_DIR/baoregon-renode-demo.elf"

echo "Built $BUILD_DIR/baoregon-renode-demo.elf"

if [ "${1:-}" = "--build-only" ]; then
    exit 0
fi

echo "--- Launching Renode ---"
exec renode --console --disable-xwt renode/bao1x_demo.resc
