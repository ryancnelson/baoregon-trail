#!/usr/bin/env python3
"""
tools/capture_qemu_snapshot.py -- Launch QEMU under -display cocoa for both:
  1) build-dos33boot/baoregon-dos33boot.elf -> docs/screenshot_dos33boot_real_rom.png
  2) build-zork1boot/baoregon-zork1boot.elf -> docs/screenshot_zork1boot_real_rom.png
Captures live window snapshots via Hammerspoon hs.window:snapshot() and copies
both files to the artifact directory.
"""
import os
import sys
import time
import hashlib
import subprocess
import shutil

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
ARTIFACT_DIR = "/Volumes/T9/ryan-homedir/.gemini/antigravity-cli/brain/6b5b6f3d-522a-412e-a76b-1801d5c05485"

TARGETS = [
    ("dos33boot", os.path.join(REPO_ROOT, "build-dos33boot", "baoregon-dos33boot.elf"), "screenshot_dos33boot_real_rom.png"),
    ("zork1boot", os.path.join(REPO_ROOT, "build-zork1boot", "baoregon-zork1boot.elf"), "screenshot_zork1boot_real_rom.png")
]

captured_hashes = {}  # png_filename -> md5, used at the end to catch the
                       # exact bug that shipped twice already: two DIFFERENT
                       # targets producing byte-for-byte IDENTICAL screenshots
                       # because the previous target's QEMU window was still
                       # open when the next one's snapshot was taken.

for name, elf_path, png_filename in TARGETS:
    if not os.path.exists(elf_path):
        print(f"ERROR: ELF not found at {elf_path}")
        continue

    output_png = os.path.join(REPO_ROOT, "docs", png_filename)
    artifact_png = os.path.join(ARTIFACT_DIR, png_filename)

    print(f"\n==========================================")
    print(f"Testing target: {name}")
    print(f"Launching QEMU for {elf_path} under cocoa display...")
    qemu_proc = subprocess.Popen([
        "qemu-system-riscv32",
        "-M", "virt",
        "-bios", "none",
        "-device", "ramfb",
        "-display", "cocoa",
        "-serial", "stdio",
        "-kernel", elf_path
    ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    print(f"Waiting 10 seconds for {name} boot to execute and settle...")
    time.sleep(10)

    lua_script = f"""
    local app = hs.application.find('qemu-system-riscv32') or hs.application.get('QEMU')
    if not app then
        local apps = hs.application.runningApplications()
        for _, a in ipairs(apps) do
            if a:name():lower():find('qemu') then
                app = a
                break
            end
        end
    end
    if app then
        local win = app:mainWindow() or app:allWindows()[1]
        if win then
            win:focus()
            hs.timer.usleep(500000)
            local img = win:snapshot()
            if img then
                img:saveToFile('{output_png}')
                print('SNAPSHOT_SUCCESS: ' .. win:title())
            else
                print('SNAPSHOT_ERROR: snapshot returned nil')
            end
        else
            print('SNAPSHOT_ERROR: no window found for app')
        end
    else
        print('SNAPSHOT_ERROR: QEMU app not found')
    end
    """

    print("Executing Hammerspoon Lua script to snapshot QEMU window...")
    res = subprocess.run(["hs", "-c", lua_script], capture_output=True, text=True)
    print("Hammerspoon stdout:", res.stdout.strip())

    print("Terminating QEMU...")
    qemu_proc.terminate()
    try:
        qemu_proc.wait(timeout=3)
    except subprocess.TimeoutExpired:
        qemu_proc.kill()
        qemu_proc.wait(timeout=3)

    # Real bug fixed here (found by Danny/Maestro after two separate runs
    # produced byte-for-byte IDENTICAL screenshots for different targets,
    # confirmed via md5): terminate() returning doesn't guarantee the
    # window server has actually torn down the QEMU cocoa window yet.
    # Without a real wait, the NEXT loop iteration's Hammerspoon snapshot
    # can grab a still-lingering window from the PREVIOUS target instead
    # of (or blended with) the new one. Poll for the specific QEMU window
    # to actually disappear before moving to the next target.
    print("Waiting for QEMU window to fully close before next target...")
    for _ in range(20):
        check = subprocess.run(
            ["osascript", "-e",
             'tell application "System Events" to get name of every process whose name contains "qemu"'],
            capture_output=True, text=True
        )
        if "qemu" not in check.stdout.lower():
            break
        time.sleep(0.5)
    else:
        print("WARNING: qemu-system-riscv32 process still visible after 10s wait -- proceeding anyway, results may be unreliable.")

    if os.path.exists(output_png):
        print(f"SUCCESS: Snapshot saved to {output_png} (size: {os.path.getsize(output_png)} bytes)")
        with open(output_png, "rb") as f:
            captured_hashes[png_filename] = hashlib.md5(f.read()).hexdigest()
        if os.path.exists(ARTIFACT_DIR):
            shutil.copy(output_png, artifact_png)
            print(f"Copied to artifact dir: {artifact_png}")
    else:
        print(f"FAILED: Output PNG {output_png} was not created.")

# Hard, automatic check: refuse to let this class of bug ship silently
# again. If any two distinct targets produced byte-for-byte identical
# screenshots, something in the capture pipeline grabbed a stale/wrong
# window -- fail loudly instead of committing misleading "evidence".
if len(set(captured_hashes.values())) < len(captured_hashes):
    print("\n*** FATAL: two or more captured screenshots are BYTE-FOR-BYTE IDENTICAL. ***")
    for fname, h in captured_hashes.items():
        print(f"  {fname}: md5={h}")
    print("This means the capture pipeline grabbed the same window twice -- do NOT")
    print("commit these files. Investigate the window-close/re-launch timing above.")
    sys.exit(1)
else:
    print("\nAll captured screenshots have distinct content -- safe to commit.")
