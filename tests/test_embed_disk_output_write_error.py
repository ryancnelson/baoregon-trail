"""
tests/test_embed_disk_output_write_error.py -- RED test: does
embed_disk.py's CLI handle a write failure (nonexistent output
directory) gracefully, matching the clean "error: ..." + exit 1
pattern already established for its own read path
(read_dsk_bytes/DiskImageSizeError/OSError) and the other three tools
(build_cartridge_image.py, create_sample_boot_dsk.py,
check_linker_placement.py), instead of a raw Python traceback?
"""
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent.parent / "tools"
DOS33_DISK_IMAGE_SIZE = 35 * 16 * 256


class TestEmbedDiskOutputWriteError(unittest.TestCase):
    def test_nonexistent_output_directory_exits_cleanly_no_traceback(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            dsk_path = Path(tmpdir) / "valid.dsk"
            dsk_path.write_bytes(bytes(DOS33_DISK_IMAGE_SIZE))
            bad_output = Path(tmpdir) / "nonexistent_subdir" / "out.h"

            result = subprocess.run(
                [sys.executable, str(TOOLS_DIR / "embed_disk.py"),
                 str(dsk_path), "test_array", "-o", str(bad_output)],
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
