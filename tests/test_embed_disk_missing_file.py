"""
tests/test_embed_disk_missing_file.py -- RED-GREEN: embed_disk.py's CLI
must fail cleanly (exit 1, one-line stderr message) on a missing .dsk
file, not crash with a raw Python traceback.

Gap found by manual invocation: `python3 tools/embed_disk.py
/nonexistent.dsk name` currently crashes with an unhandled
FileNotFoundError traceback from Path.read_bytes() -- main() only
catches DiskImageSizeError, not the file-not-found case, even though
both are "bad input" errors that should get the same clean
error-and-exit-1 treatment. tools/build_cartridge_image.py's main()
already handles this gracefully (broad except in its CLI wrapper); this
brings embed_disk.py's CLI up to the same standard.
"""
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
EMBED_DISK_PY = REPO_ROOT / "tools" / "embed_disk.py"


class TestEmbedDiskMissingFileCli(unittest.TestCase):
    def test_missing_dsk_file_exits_1_with_clean_message_no_traceback(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            missing_path = Path(tmpdir) / "does_not_exist.dsk"
            result = subprocess.run(
                [sys.executable, str(EMBED_DISK_PY), str(missing_path), "some_array"],
                capture_output=True, text=True,
            )

        self.assertEqual(result.returncode, 1,
                          "missing .dsk file must exit 1, not crash with an "
                          "unhandled exception's default exit code")
        self.assertNotIn("Traceback (most recent call last)", result.stderr,
                          "must not leak a raw Python traceback to the user -- "
                          "a clean one-line error message is expected instead")
        self.assertIn("error", result.stderr.lower(),
                       "stderr should contain a recognizable error message")

    def test_wrong_sized_dsk_file_still_exits_1_with_clean_message(self):
        """Regression guard: the existing DiskImageSizeError handling
        must keep working after adding the missing-file handling."""
        with tempfile.TemporaryDirectory() as tmpdir:
            bad_path = Path(tmpdir) / "too_small.dsk"
            bad_path.write_bytes(b"\x00" * 100)
            result = subprocess.run(
                [sys.executable, str(EMBED_DISK_PY), str(bad_path), "some_array"],
                capture_output=True, text=True,
            )

        self.assertEqual(result.returncode, 1)
        self.assertNotIn("Traceback (most recent call last)", result.stderr)
        self.assertIn("error", result.stderr.lower())


if __name__ == "__main__":
    unittest.main()
