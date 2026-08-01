"""
tests/test_build_cartridge_image.py -- unit tests for tools/build_cartridge_image.py
"""
import unittest
from pathlib import Path
import tempfile
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
from build_cartridge_image import (
    build_cartridge_image,
    CARTRIDGE_SLOT_COUNT,
    CARTRIDGE_SLOT_SIZE,
    SlotIndexError,
    ERASED_FILL_BYTE,
)
from embed_disk import DOS33_DISK_IMAGE_SIZE, DiskImageSizeError


class TestBuildCartridgeImage(unittest.TestCase):
    def test_unpopulated_cartridge_is_all_0xff(self):
        img = build_cartridge_image({})
        self.assertEqual(len(img), CARTRIDGE_SLOT_COUNT * CARTRIDGE_SLOT_SIZE)
        self.assertEqual(set(img), {ERASED_FILL_BYTE})

    def test_single_slot_population(self):
        with tempfile.NamedTemporaryFile(delete=False) as tmp:
            tmp.write(b"A" * DOS33_DISK_IMAGE_SIZE)
            tmp_path = Path(tmp.name)

        try:
            img = build_cartridge_image({0: tmp_path})
            self.assertEqual(len(img), CARTRIDGE_SLOT_COUNT * CARTRIDGE_SLOT_SIZE)
            self.assertEqual(img[:DOS33_DISK_IMAGE_SIZE], b"A" * DOS33_DISK_IMAGE_SIZE)
            self.assertEqual(set(img[DOS33_DISK_IMAGE_SIZE:]), {ERASED_FILL_BYTE})
        finally:
            tmp_path.unlink()

    def test_rejects_invalid_slot_index(self):
        with tempfile.NamedTemporaryFile(delete=False) as tmp:
            tmp.write(b"A" * DOS33_DISK_IMAGE_SIZE)
            tmp_path = Path(tmp.name)

        try:
            with self.assertRaises(SlotIndexError):
                build_cartridge_image({99: tmp_path})
        finally:
            tmp_path.unlink()

    def test_rejects_wrong_sized_dsk(self):
        with tempfile.NamedTemporaryFile(delete=False) as tmp:
            tmp.write(b"SHORT")
            tmp_path = Path(tmp.name)

        try:
            with self.assertRaises(DiskImageSizeError):
                build_cartridge_image({0: tmp_path})
        finally:
            tmp_path.unlink()


if __name__ == "__main__":
    unittest.main()
