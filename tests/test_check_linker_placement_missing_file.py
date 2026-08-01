"""
tests/test_check_linker_placement_missing_file.py -- RED-GREEN:
tools/check_linker_placement.py's CLI must fail cleanly (exit non-zero,
one-line stderr message) when the given ELF path doesn't exist, not
crash with a raw subprocess.CalledProcessError traceback.

Gap found by manual invocation (same class of bug as
embed_disk.py's missing-file crash, fixed 2026-08-01): `python3
tools/check_linker_placement.py /nonexistent.elf` currently crashes with
an unhandled subprocess.CalledProcessError from readelf failing on a
nonexistent file, instead of a clean "error: ..." message and exit 1/2.
"""
import subprocess
import sys
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
CHECK_LINKER_PLACEMENT_PY = REPO_ROOT / "tools" / "check_linker_placement.py"


class TestCheckLinkerPlacementMissingFile(unittest.TestCase):
    def test_missing_elf_file_exits_nonzero_with_clean_message_no_traceback(self):
        result = subprocess.run(
            [sys.executable, str(CHECK_LINKER_PLACEMENT_PY), "/tmp/does_not_exist_baoregon.elf"],
            capture_output=True, text=True,
        )

        self.assertNotEqual(result.returncode, 0,
                             "missing ELF file must exit non-zero")
        self.assertNotIn("Traceback (most recent call last)", result.stderr,
                          "must not leak a raw Python traceback -- a clean "
                          "one-line error message is expected instead")
        self.assertIn("error", result.stderr.lower(),
                       "stderr should contain a recognizable error message")


if __name__ == "__main__":
    unittest.main()
