#!/usr/bin/env python3
"""
tools/run_bio_display_palette_biosim.py -- build and run
bio-sim-tests/sw/bio_display_palette against the REAL bio-sim Verilator
RTL simulation (github.com/baochip/bio-sim), verifying its RGB565 output
matches src/bio_display.c's already host/cross-compile-verified
bio_display_color_to_rgb565() palette, for all 6 hires_color_t values.

Requires:
  - A checked-out bio-sim repo (BIO_SIM_DIR env var, or ../bio-sim by
    default) with `make build` already run (needs Verilator + a Python
    venv with ziglang -- see bio-sim-tests/README.md).
  - This script copies bio-sim-tests/sw/bio_display_palette and
    configs/bio_display_palette.jsonc into bio-sim's own sw/ and
    configs/ directories (bio-sim's build tooling expects sources
    there), builds the BIO binary with ziglang, and runs it under
    bio-sim, then parses fifo_read output and checks it against the
    expected table.

This is a REAL execution test of hand-ported hardware-target code
against the actual bio_bdma RTL (via Verilator), not a host-native mock.
"""
import json
import os
import re
import shutil
import subprocess
import sys

# Must exactly match src/video_apple2.h's hires_color_t ordinals and
# src/bio_display.c's g_color_to_rgb565[] table.
EXPECTED_RGB565 = [
    0x0000,  # HIRES_COLOR_BLACK
    0x07E0,  # HIRES_COLOR_GREEN
    0x781F,  # HIRES_COLOR_VIOLET
    0xFC00,  # HIRES_COLOR_ORANGE
    0x001F,  # HIRES_COLOR_BLUE
    0xFFFF,  # HIRES_COLOR_WHITE
]

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIO_SIM_TESTS_DIR = os.path.join(REPO_ROOT, "bio-sim-tests")


def fail(msg):
    print(f"FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


def main():
    bio_sim_dir = os.environ.get(
        "BIO_SIM_DIR", os.path.join(REPO_ROOT, "..", "bio-sim")
    )
    bio_sim_dir = os.path.abspath(bio_sim_dir)

    if not os.path.isdir(bio_sim_dir):
        fail(
            f"bio-sim not found at {bio_sim_dir}. Clone it: "
            f"git clone https://github.com/baochip/bio-sim.git {bio_sim_dir}\n"
            f"Then run `make build` inside it (needs Verilator). "
            f"Override the path with BIO_SIM_DIR=/path/to/bio-sim."
        )

    bio_sim_exe = os.path.join(bio_sim_dir, "build", "bio_sim")
    if not os.path.isfile(bio_sim_exe):
        fail(
            f"{bio_sim_exe} not built yet. Run `make build` inside "
            f"{bio_sim_dir} first (requires Verilator)."
        )

    # Copy our firmware source + config into bio-sim's own tree (its
    # build.zig / bio_sim binary expect sw/<module>/main.c and
    # configs/<name>.jsonc relative to bio-sim's own root).
    src_sw = os.path.join(BIO_SIM_TESTS_DIR, "sw", "bio_display_palette")
    dst_sw = os.path.join(bio_sim_dir, "sw", "bio_display_palette")
    shutil.rmtree(dst_sw, ignore_errors=True)
    shutil.copytree(src_sw, dst_sw)

    src_cfg = os.path.join(
        BIO_SIM_TESTS_DIR, "configs", "bio_display_palette.jsonc"
    )
    dst_cfg = os.path.join(bio_sim_dir, "configs", "bio_display_palette.jsonc")
    shutil.copy(src_cfg, dst_cfg)

    # Build the BIO binary with ziglang python module.
    biovenv_python = "/tmp/biovenv/bin/python3"
    default_python = biovenv_python if os.path.exists(biovenv_python) else sys.executable
    ziglang_python = os.environ.get("ZIGLANG_PYTHON", default_python)
    build_cmd = [
        ziglang_python,
        "-m",
        "ziglang",
        "build",
        "-Dmodule=bio_display_palette",
    ]
    result = subprocess.run(
        build_cmd, cwd=os.path.join(bio_sim_dir, "sw"),
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        print(result.stdout)
        print(result.stderr, file=sys.stderr)
        fail("ziglang build of bio_display_palette failed")

    # Run under bio-sim.
    run_result = subprocess.run(
        [bio_sim_exe, "configs/bio_display_palette.jsonc"],
        cwd=bio_sim_dir, capture_output=True, text=True,
    )
    output = run_result.stdout + run_result.stderr

    # Parse "[fifo_read]   [0] = 0x0000XXXX" lines in order.
    got_values = [
        int(m, 16)
        for m in re.findall(r"\[fifo_read\]\s+\[0\] = 0x([0-9a-fA-F]+)", output)
    ]

    if len(got_values) != len(EXPECTED_RGB565):
        print(output)
        fail(
            f"expected {len(EXPECTED_RGB565)} fifo_read results, "
            f"got {len(got_values)}"
        )

    failures = 0
    color_names = ["BLACK", "GREEN", "VIOLET", "ORANGE", "BLUE", "WHITE"]
    for i, (got, expected) in enumerate(zip(got_values, EXPECTED_RGB565)):
        status = "PASS" if got == expected else "FAIL"
        if got != expected:
            failures += 1
        print(
            f"{status}: {color_names[i]} -> got 0x{got:04X}, "
            f"expected 0x{expected:04X}"
        )

    if failures:
        fail(f"{failures} color(s) mismatched against bio_display.c's palette")

    print("All 6 colors matched src/bio_display.c's palette under real bio-sim execution.")


if __name__ == "__main__":
    main()
