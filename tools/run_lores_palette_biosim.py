#!/usr/bin/env python3
"""
tools/run_lores_palette_biosim.py -- build and run
bio-sim-tests/sw/lores_palette against the REAL bio-sim Verilator RTL
simulation (github.com/baochip/bio-sim), verifying its RGB565 output
matches src/lores_apple2.c's already host/cross-compile-verified
lores_color_to_rgb565() palette, for all 16 Lo-Res color indices.

Mirrors tools/run_bio_display_palette_biosim.py exactly (see that
script's docstring for the shared setup requirements).
"""
import os
import re
import shutil
import subprocess
import sys

# Must exactly match src/lores_apple2.c's palette[] table.
EXPECTED_RGB565 = [
    0x0000,  #  0 Black
    0x9000,  #  1 Deep Red
    0x000D,  #  2 Dark Blue
    0xA0B8,  #  3 Purple
    0x0320,  #  4 Dark Green
    0x738E,  #  5 Gray 1
    0x055F,  #  6 Medium Blue
    0x7BFF,  #  7 Light Blue
    0x5300,  #  8 Brown
    0xFC60,  #  9 Orange
    0xC618,  # 10 Gray 2
    0xFB56,  # 11 Pink
    0x07E0,  # 12 Green
    0xFFA0,  # 13 Yellow
    0x7FF5,  # 14 Aqua
    0xFFFF,  # 15 White
]
COLOR_NAMES = [
    "Black", "Deep Red", "Dark Blue", "Purple", "Dark Green", "Gray 1",
    "Medium Blue", "Light Blue", "Brown", "Orange", "Gray 2", "Pink",
    "Green", "Yellow", "Aqua", "White",
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

    src_sw = os.path.join(BIO_SIM_TESTS_DIR, "sw", "lores_palette")
    dst_sw = os.path.join(bio_sim_dir, "sw", "lores_palette")
    shutil.rmtree(dst_sw, ignore_errors=True)
    shutil.copytree(src_sw, dst_sw)

    src_cfg = os.path.join(BIO_SIM_TESTS_DIR, "configs", "lores_palette.jsonc")
    dst_cfg = os.path.join(bio_sim_dir, "configs", "lores_palette.jsonc")
    shutil.copy(src_cfg, dst_cfg)

    biovenv_python = "/tmp/biovenv/bin/python3"
    default_python = biovenv_python if os.path.exists(biovenv_python) else sys.executable
    ziglang_python = os.environ.get("ZIGLANG_PYTHON", default_python)
    build_cmd = [ziglang_python, "-m", "ziglang", "build", "-Dmodule=lores_palette"]
    result = subprocess.run(
        build_cmd, cwd=os.path.join(bio_sim_dir, "sw"),
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        print(result.stdout)
        print(result.stderr, file=sys.stderr)
        fail("ziglang build of lores_palette failed")

    run_result = subprocess.run(
        [bio_sim_exe, "configs/lores_palette.jsonc"],
        cwd=bio_sim_dir, capture_output=True, text=True,
    )
    output = run_result.stdout + run_result.stderr

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
    for i, (got, expected) in enumerate(zip(got_values, EXPECTED_RGB565)):
        status = "PASS" if got == expected else "FAIL"
        if got != expected:
            failures += 1
        print(f"{status}: {COLOR_NAMES[i]} -> got 0x{got:04X}, expected 0x{expected:04X}")

    if failures:
        fail(f"{failures} color(s) mismatched against lores_apple2.c's palette")

    print("All 16 Lo-Res colors matched src/lores_apple2.c's palette under real bio-sim execution.")


if __name__ == "__main__":
    main()
