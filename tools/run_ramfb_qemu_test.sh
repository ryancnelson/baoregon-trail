#!/bin/bash
# tools/run_ramfb_qemu_test.sh -- build and execute test_ramfb_fwcfg.c under QEMU
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build-qemu"
CROSS_PREFIX="${CROSS_PREFIX:-riscv64-elf-}"
CC="${CROSS_PREFIX}gcc"

mkdir -p "$BUILD_DIR"

"$CC" -march=rv32imac -mabi=ilp32 -std=c99 -Wall -Wextra -Os -ffreestanding -fno-builtin -nostartfiles -g -c -o "$BUILD_DIR/crt0.o" "$REPO_ROOT/src/crt0.S"
"$CC" -march=rv32imac -mabi=ilp32 -std=c99 -Wall -Wextra -Os -ffreestanding -fno-builtin -nostartfiles -g -c -o "$BUILD_DIR/test_ramfb_fwcfg.o" "$REPO_ROOT/tools/test_ramfb_fwcfg.c"
"$CC" -march=rv32imac -mabi=ilp32 -nostartfiles -nostdlib -T "$REPO_ROOT/linker-qemu.ld" -o "$BUILD_DIR/test_ramfb_fwcfg.elf" "$BUILD_DIR/crt0.o" "$BUILD_DIR/test_ramfb_fwcfg.o"

OUTPUT=$(timeout 5 qemu-system-riscv32 -M virt -bios none -device ramfb -display none -serial stdio -kernel "$BUILD_DIR/test_ramfb_fwcfg.elf" 2>&1 || true)

if echo "$OUTPUT" | grep -q "RESULT=PASS"; then
    echo "PASS: ramfb_fwcfg_selector_lookup (RV32IMAC, QEMU virt execution)"
    exit 0
else
    echo "FAIL: ramfb_fwcfg_selector_lookup -- output missing RESULT=PASS:"
    echo "$OUTPUT"
    exit 1
fi
