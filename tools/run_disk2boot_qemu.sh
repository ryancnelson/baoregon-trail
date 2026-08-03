#!/bin/bash
# tools/run_disk2boot_qemu.sh -- build and run main_qemu_disk2boot.c
# under real QEMU with a live ramfb display. This is the real, confirmed-
# working DOS 3.3 stretch-goal demo (disks/dos33_sample.dsk, a synthetic
# sample disk built by this project's own tools, with a minimal stub
# ROM + the real text_apple2.c glyph renderer) -- distinct from
# main_qemu_dos33boot.c (real Apple DOS 3.3 Master disk + real Apple IIe
# Monitor-only ROM, which has NO Applesoft BASIC and has never actually
# produced readable banner text -- see NEXT_STEPS.md's extensive
# "still-open gap" discussion). Mirrors tools/run_loderunner_qemu.sh's
# pattern.
#
# Usage:
#   tools/run_disk2boot_qemu.sh                  # build + run headless (-display none)
#   tools/run_disk2boot_qemu.sh --cocoa          # build + run with a real window (macOS)
#   tools/run_disk2boot_qemu.sh --build-only      # just build the ELF, don't run
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build-qemu"
CROSS_PREFIX="${CROSS_PREFIX:-riscv64-elf-}"
CC="${CROSS_PREFIX}gcc"

CFLAGS="-march=rv32imac -mabi=ilp32 -std=c99 -Wall -Wextra -Os -ffreestanding -fno-builtin -nostartfiles -g -I$REPO_ROOT/src"
LDFLAGS="-march=rv32imac -mabi=ilp32 -nostartfiles -nostdlib -T $REPO_ROOT/linker-qemu.ld"

mkdir -p "$BUILD_DIR"

SOURCES=(
    "$REPO_ROOT/src/crt0.S"
    "$REPO_ROOT/src/main_qemu_disk2boot.c"
    "$REPO_ROOT/src/cpu6502.c"
    "$REPO_ROOT/src/apple2_mem.c"
    "$REPO_ROOT/src/disk2_controller.c"
    "$REPO_ROOT/src/disk_sector_layout.c"
    "$REPO_ROOT/src/disk_trap.c"
    "$REPO_ROOT/src/bunnie_audio.c"
    "$REPO_ROOT/src/video_apple2.c"
    "$REPO_ROOT/src/lores_apple2.c"
    "$REPO_ROOT/src/text_apple2.c"
    "$REPO_ROOT/src/bio_display.c"
    "$REPO_ROOT/src/uart_keyboard_bridge.c"
    "$REPO_ROOT/tools/ramfb_display.c"
)

OBJECTS=()
for src in "${SOURCES[@]}"; do
    base="$(basename "$src")"
    obj="$BUILD_DIR/disk2boot_${base%.*}.o"
    "$CC" $CFLAGS -c -o "$obj" "$src"
    OBJECTS+=("$obj")
done

ELF="$BUILD_DIR/baoregon-disk2boot-qemu.elf"
"$CC" $LDFLAGS -o "$ELF" "${OBJECTS[@]}"
echo "Built $ELF"

if [[ "${1:-}" == "--build-only" ]]; then
    exit 0
fi

if [[ "${1:-}" == "--cocoa" ]]; then
    exec qemu-system-riscv32 -M virt -bios none -device ramfb -display cocoa -serial stdio -kernel "$ELF"
else
    exec timeout "${QEMU_TIMEOUT:-30}" qemu-system-riscv32 -M virt -bios none -device ramfb -display none -serial stdio -kernel "$ELF"
fi
