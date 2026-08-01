"""
tests/test_embed_disk.py -- round-trip test for tools/embed_disk.py.

Per CLAUDE.md: "test round-trip correctness (embed a known .dsk, extract it
back byte-for-byte) before trusting it on the real Oregon Trail image."

Uses a synthetic 143360-byte image (not a real .dsk) so this test has no
external file dependency and pins down exact byte-for-byte round-trip
correctness against a fully known input.
"""
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))

import embed_disk  # noqa: E402


def make_synthetic_disk_image() -> bytes:
    """Deterministic, fully-known 140 KiB image: byte at offset N == N & 0xFF.

    Distinct-within-256-byte-window values make off-by-one/truncation bugs
    in the embed/extract round trip easy to spot.
    """
    return bytes(i & 0xFF for i in range(embed_disk.DOS33_DISK_IMAGE_SIZE))


class TestReadDskBytes(unittest.TestCase):
    def test_reads_correctly_sized_image(self):
        data = make_synthetic_disk_image()
        with tempfile.NamedTemporaryFile(suffix=".dsk") as f:
            f.write(data)
            f.flush()
            result = embed_disk.read_dsk_bytes(Path(f.name))
        self.assertEqual(result, data)

    def test_rejects_wrong_sized_image(self):
        with tempfile.NamedTemporaryFile(suffix=".dsk") as f:
            f.write(b"\x00" * 100)  # too short
            f.flush()
            with self.assertRaises(embed_disk.DiskImageSizeError):
                embed_disk.read_dsk_bytes(Path(f.name))


class TestEmbedExtractRoundTrip(unittest.TestCase):
    def test_round_trip_byte_for_byte(self):
        """The core requirement from CLAUDE.md: embed a known .dsk, extract
        it back, and it must match byte-for-byte."""
        original = make_synthetic_disk_image()

        header = embed_disk.embed_to_c_header(original, "oregon_trail_dsk")
        extracted = embed_disk.extract_from_c_header(header, "oregon_trail_dsk")

        self.assertEqual(
            extracted, original,
            "round-trip byte mismatch: embed_to_c_header -> extract_from_c_header "
            "did not reproduce the original image byte-for-byte",
        )

    def test_round_trip_preserves_length_exactly(self):
        original = make_synthetic_disk_image()
        header = embed_disk.embed_to_c_header(original, "oregon_trail_dsk")
        extracted = embed_disk.extract_from_c_header(header, "oregon_trail_dsk")
        self.assertEqual(len(extracted), len(original))

    def test_header_declares_valid_c_array_name(self):
        original = make_synthetic_disk_image()
        header = embed_disk.embed_to_c_header(original, "oregon_trail_dsk")
        self.assertIn("oregon_trail_dsk", header)
        self.assertIn("const uint8_t oregon_trail_dsk", header)

    def test_header_declares_length_constant_matching_input(self):
        original = make_synthetic_disk_image()
        header = embed_disk.embed_to_c_header(original, "oregon_trail_dsk")
        self.assertIn(f"oregon_trail_dsk_len = {len(original)}", header)

    def test_rejects_invalid_c_identifier(self):
        original = make_synthetic_disk_image()
        with self.assertRaises(ValueError):
            embed_disk.embed_to_c_header(original, "not a valid identifier")

    def test_round_trip_on_small_edge_case_all_zero_bytes(self):
        """All-zero bytes are a common source of hex-formatting bugs
        (e.g. accidentally treating 0x00 as falsy/empty and skipping it)."""
        original = bytes(embed_disk.DOS33_DISK_IMAGE_SIZE)  # all zero
        header = embed_disk.embed_to_c_header(original, "all_zero_dsk")
        extracted = embed_disk.extract_from_c_header(header, "all_zero_dsk")
        self.assertEqual(extracted, original)

    def test_round_trip_on_all_0xff_bytes(self):
        original = bytes([0xFF]) * embed_disk.DOS33_DISK_IMAGE_SIZE
        header = embed_disk.embed_to_c_header(original, "all_ff_dsk")
        extracted = embed_disk.extract_from_c_header(header, "all_ff_dsk")
        self.assertEqual(extracted, original)


if __name__ == "__main__":
    unittest.main()
