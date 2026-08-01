"""
tests/test_build_cartridge_image_constants.py -- supplementary tests for
tools/build_cartridge_image.py, filling gaps not covered by
tests/test_build_cartridge_image.py:

1. CARTRIDGE_SLOT_COUNT / CARTRIDGE_SLOT_SIZE must match the REAL
   src/cartridge_layout.h constants (parsed from the actual header text,
   not hand-copied) -- a silent drift here would build a cartridge image
   with the wrong slot boundaries for the real linker/runtime layout.
2. Multiple non-adjacent slots populated simultaneously, out of insertion
   order, must land at the correct fixed byte offsets (index * slot_size)
   regardless of dict iteration order -- existing tests only cover 0 or 1
   populated slot, not the multi-slot case that real cartridge flashing
   actually needs.
"""
import re
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))

from build_cartridge_image import (  # noqa: E402
    build_cartridge_image,
    CARTRIDGE_SLOT_COUNT,
    CARTRIDGE_SLOT_SIZE,
    ERASED_FILL_BYTE,
)

REPO_ROOT = Path(__file__).resolve().parent.parent
CARTRIDGE_LAYOUT_H = REPO_ROOT / "src" / "cartridge_layout.h"


def _parse_c_int_define(header_text: str, name: str) -> int:
    """Extract an integer #define value (decimal or 0x-hex, optional 'u'
    suffix) from cartridge_layout.h's actual text -- not hand-copied."""
    match = re.search(rf"#define\s+{re.escape(name)}\s+(\S+)", header_text)
    if not match:
        raise AssertionError(f"could not find #define {name} in {CARTRIDGE_LAYOUT_H}")
    literal = match.group(1).rstrip("uU")
    return int(literal, 0)  # base=0 autodetects 0x.. hex vs decimal


class TestConstantsMatchRealHeader(unittest.TestCase):
    def test_slot_count_matches_cartridge_layout_h(self):
        header_text = CARTRIDGE_LAYOUT_H.read_text()
        real_count = _parse_c_int_define(header_text, "CARTRIDGE_SLOT_COUNT")
        self.assertEqual(CARTRIDGE_SLOT_COUNT, real_count)

    def test_slot_size_matches_cartridge_layout_h(self):
        header_text = CARTRIDGE_LAYOUT_H.read_text()
        real_size = _parse_c_int_define(header_text, "CARTRIDGE_SLOT_SIZE")
        self.assertEqual(CARTRIDGE_SLOT_SIZE, real_size)


class TestMultiSlotPopulation(unittest.TestCase):
    def _make_dsk(self, tmpdir: Path, fill_byte: int) -> Path:
        path = tmpdir / f"game_{fill_byte:02x}.dsk"
        path.write_bytes(bytes([fill_byte]) * CARTRIDGE_SLOT_SIZE)
        return path

    def test_non_adjacent_slots_populated_out_of_order_land_at_correct_offsets(self):
        with tempfile.TemporaryDirectory() as tmpdir_str:
            tmpdir = Path(tmpdir_str)
            # Populate slots 4, 0, and 2 -- non-adjacent, and inserted out
            # of numeric order into the dict -- to prove output byte
            # position is fixed by slot index * CARTRIDGE_SLOT_SIZE, not
            # by dict/insertion order.
            slot_paths = {
                4: self._make_dsk(tmpdir, fill_byte=0x44),
                0: self._make_dsk(tmpdir, fill_byte=0x11),
                2: self._make_dsk(tmpdir, fill_byte=0x22),
            }
            image = build_cartridge_image(slot_paths)

            self.assertEqual(len(image), CARTRIDGE_SLOT_COUNT * CARTRIDGE_SLOT_SIZE)

            def slot_bytes(index):
                start = index * CARTRIDGE_SLOT_SIZE
                end = start + CARTRIDGE_SLOT_SIZE
                return image[start:end]

            self.assertTrue(all(b == 0x11 for b in slot_bytes(0)), "slot 0 mismatched")
            self.assertTrue(all(b == ERASED_FILL_BYTE for b in slot_bytes(1)), "slot 1 should be unfilled")
            self.assertTrue(all(b == 0x22 for b in slot_bytes(2)), "slot 2 mismatched")
            self.assertTrue(all(b == ERASED_FILL_BYTE for b in slot_bytes(3)), "slot 3 should be unfilled")
            self.assertTrue(all(b == 0x44 for b in slot_bytes(4)), "slot 4 mismatched")
            self.assertTrue(all(b == ERASED_FILL_BYTE for b in slot_bytes(5)), "slot 5 should be unfilled")

    def test_all_six_slots_populated_simultaneously(self):
        with tempfile.TemporaryDirectory() as tmpdir_str:
            tmpdir = Path(tmpdir_str)
            slot_paths = {
                i: self._make_dsk(tmpdir, fill_byte=0x10 + i)
                for i in range(CARTRIDGE_SLOT_COUNT)
            }
            image = build_cartridge_image(slot_paths)

            self.assertEqual(len(image), CARTRIDGE_SLOT_COUNT * CARTRIDGE_SLOT_SIZE)
            for i in range(CARTRIDGE_SLOT_COUNT):
                start = i * CARTRIDGE_SLOT_SIZE
                end = start + CARTRIDGE_SLOT_SIZE
                expected_fill = 0x10 + i
                self.assertTrue(
                    all(b == expected_fill for b in image[start:end]),
                    f"slot {i} does not match its assigned game's fill byte",
                )
            # No 0xFF bytes should remain anywhere once every slot is filled.
            self.assertNotIn(ERASED_FILL_BYTE, image)


if __name__ == "__main__":
    unittest.main()
