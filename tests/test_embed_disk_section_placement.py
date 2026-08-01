"""
tests/test_embed_disk_section_placement.py -- RED-GREEN: embed_to_c_header()
must place the embedded disk array in the .dsk_images linker section, not
let it fall into .rodata by default.

Gap found by inspection: linker.ld defines a dedicated .dsk_images output
section specifically so embedded game images are placed in a
budget-visible, independently-relocatable ReRAM region (see linker.ld's
own comment: "so the multi-game cartridge loader can reason
about/relocate this region independently... and so size/readelf output
makes disk-image footprint visible at a glance"). But nothing in
tools/embed_disk.py ever emitted a
__attribute__((section(".dsk_images"))) annotation -- every embedded
game array would silently link into plain .rodata instead, defeating the
entire reason that output section exists. This test locks in the fix.
"""
import re
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))

import embed_disk  # noqa: E402


def make_synthetic_disk_image() -> bytes:
    return bytes(i & 0xFF for i in range(embed_disk.DOS33_DISK_IMAGE_SIZE))


class TestDskImagesSectionPlacement(unittest.TestCase):
    def test_array_definition_has_dsk_images_section_attribute(self):
        original = make_synthetic_disk_image()
        header = embed_disk.embed_to_c_header(original, "oregon_trail_dsk")

        # Must find: const uint8_t oregon_trail_dsk[N]
        #            __attribute__((section(".dsk_images"))) = { ... };
        # (or the attribute placed elsewhere in the same declaration --
        # what matters is GCC/Clang actually see it attached to the array
        # definition, not merely present as a comment anywhere in the file.)
        pattern = re.compile(
            r"const\s+uint8_t\s+oregon_trail_dsk\s*\[\s*\d+\s*\]\s*"
            r"DSK_SECTION\s*=",
        )
        self.assertRegex(
            header, pattern,
            "embed_to_c_header() output must place the array in the "
            ".dsk_images linker section via DSK_SECTION macro "
            "-- otherwise it silently falls into .rodata, defeating "
            "linker.ld's dedicated .dsk_images output section.",
        )

    def test_round_trip_still_works_with_section_attribute_present(self):
        """The section attribute must not break extract_from_c_header()'s
        parsing -- round-trip correctness (the core CLAUDE.md requirement)
        must survive this change."""
        original = make_synthetic_disk_image()
        header = embed_disk.embed_to_c_header(original, "oregon_trail_dsk")
        extracted = embed_disk.extract_from_c_header(header, "oregon_trail_dsk")
        self.assertEqual(extracted, original)

    def test_length_constant_is_not_placed_in_dsk_images(self):
        """Only the byte array itself belongs in .dsk_images -- the
        _len constant is a tiny uint32_t and belongs in ordinary .rodata
        (no reason to burn ReRAM budget-tracking attention on a single
        4-byte constant); this pins that only one section attribute
        appears, attached to the array, not the length declaration."""
        original = make_synthetic_disk_image()
        header = embed_disk.embed_to_c_header(original, "oregon_trail_dsk")
        array_line = next(
            line for line in header.splitlines()
            if "oregon_trail_dsk[" in line
        )
        self.assertIn("DSK_SECTION", array_line)
        len_line = next(
            line for line in header.splitlines()
            if "oregon_trail_dsk_len" in line
        )
        self.assertNotIn("DSK_SECTION", len_line)
        self.assertNotIn("__attribute__", len_line)


if __name__ == "__main__":
    unittest.main()
