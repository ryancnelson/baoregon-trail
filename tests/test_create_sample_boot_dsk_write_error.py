"""
tests/test_create_sample_boot_dsk_write_error.py -- RED test: does
tools/create_sample_boot_dsk.py's CLI handle a write failure (permission
denied, missing/uncreatable parent directory) gracefully, matching the
clean "error: ..." + exit 1 pattern already established in
embed_disk.py, check_linker_placement.py, and build_cartridge_image.py,
instead of a raw Python traceback?
"""
import os
import stat
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent.parent / "tools"


class TestCreateSampleBootDskWriteError(unittest.TestCase):
    def test_permission_denied_output_dir_exits_cleanly_no_traceback(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            readonly_dir = Path(tmpdir) / "readonly"
            readonly_dir.mkdir()
            readonly_dir.chmod(stat.S_IRUSR | stat.S_IXUSR)  # read+exec only
            try:
                bad_output = readonly_dir / "out.dsk"
                result = subprocess.run(
                    [sys.executable, str(TOOLS_DIR / "create_sample_boot_dsk.py"),
                     str(bad_output)],
                    capture_output=True, text=True,
                )
            finally:
                readonly_dir.chmod(stat.S_IRWXU)  # restore for cleanup

        self.assertNotEqual(result.returncode, 0,
                             "expected nonzero exit on write failure")
        self.assertNotIn("Traceback", result.stderr,
                          f"raw Python traceback leaked to stderr:\n{result.stderr}")
        self.assertIn("error", result.stderr.lower(),
                       f"expected a clean 'error: ...' message, got:\n{result.stderr}")


if __name__ == "__main__":
    unittest.main()
