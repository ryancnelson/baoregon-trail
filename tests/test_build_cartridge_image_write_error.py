"""
tests/test_build_cartridge_image_write_error.py -- RED test: does
build_cartridge_image.py's CLI handle a write failure (e.g. output
directory doesn't exist) gracefully, matching the clean
"error: ..." + exit 1 pattern already established in embed_disk.py
and check_linker_placement.py, instead of a raw Python traceback?
"""
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent.parent / "tools"


class TestBuildCartridgeImageWriteError(unittest.TestCase):
    def test_nonexistent_output_directory_exits_cleanly_no_traceback(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            bad_output = Path(tmpdir) / "nonexistent_subdir" / "out.bin"
            result = subprocess.run(
                [sys.executable, str(TOOLS_DIR / "build_cartridge_image.py"),
                 "-o", str(bad_output)],
                capture_output=True, text=True,
            )
        self.assertNotEqual(result.returncode, 0,
                             "expected nonzero exit on write failure")
        self.assertNotIn("Traceback", result.stderr,
                          f"raw Python traceback leaked to stderr:\n{result.stderr}")
        self.assertIn("error", result.stderr.lower(),
                       f"expected a clean 'error: ...' message, got:\n{result.stderr}")


if __name__ == "__main__":
    unittest.main()
