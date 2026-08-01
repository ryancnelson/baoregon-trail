#!/bin/bash
# tools/run_bio_audio_qemu_test.sh -- build and execute bio_audio.S's real
# RV32E machine code under QEMU (bare-metal, no OS), verifying it via the
# SiFive test-finisher MMIO device at 0x100000:
#   write (0 << 16) | 0x5555  -> QEMU exits 0  (PASS)
#   write (N << 16) | 0x3333  -> QEMU exits N  (FAIL)
#
# This is a REAL execution test of the actual BIO Core 1 firmware target
# code (RV32E, -march=rv32e -mabi=ilp32e), not a host-native mock -- see
# tests/test_bio_audio_qemu.S for the test cases (mirrors
# tests/test_bunnie_audio.c's already host-verified cases, ported to the
# real target ISA).
#
# NOTE on -mno-relax: without it, the linker uses gp-relative addressing
# for .data references, but this bare-metal harness never initializes gp
# -- discovered during RED verification (2026-07-31) when the harness hung
# forever instead of failing cleanly, traced via objdump to an
# uninitialized-gp `addi a0,gp,-2044` load. -mno-relax forces absolute
# (auipc-based) addressing instead, which needs no gp setup.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build-qemu"
CROSS_PREFIX="${CROSS_PREFIX:-riscv64-elf-}"
CC="${CROSS_PREFIX}gcc"

mkdir -p "$BUILD_DIR"

"$CC" -march=rv32e -mabi=ilp32e -mno-relax -nostdlib -nostartfiles \
    -Ttext=0x80000000 \
    -o "$BUILD_DIR/test_bio_audio_qemu.elf" \
    "$REPO_ROOT/tests/test_bio_audio_qemu.S" \
    "$REPO_ROOT/src/bio_audio.S"

set +e
timeout 5 qemu-system-riscv32 -M virt -bios none -nographic \
    -kernel "$BUILD_DIR/test_bio_audio_qemu.elf"
QEMU_EXIT=$?
set -e

if [ "$QEMU_EXIT" -eq 0 ]; then
    echo "PASS: bio_audio_poll_and_apply (RV32E, QEMU virt execution)"
    exit 0
elif [ "$QEMU_EXIT" -eq 124 ]; then
    echo "FAIL: bio_audio_poll_and_apply -- QEMU timed out (test harness never reached PASS or FAIL finisher write; likely stuck in a loop or crashed)"
    exit 1
else
    echo "FAIL: bio_audio_poll_and_apply -- QEMU exited $QEMU_EXIT (test finisher reported FAIL)"
    exit 1
fi
