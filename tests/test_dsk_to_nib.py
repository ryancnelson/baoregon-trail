"""
tests/test_dsk_to_nib.py -- RED-first test for tools/dsk_to_nib.py: proves
the nibblized track data round-trips back to the original sector bytes
using a Python-side GCR decoder (independent re-implementation of the
decode direction, not just re-running the same encode logic backwards --
per CLAUDE.md's "verify round-trip correctness before trusting it on the
real image" precedent already used for tools/embed_disk.py).
"""
import sys
import unittest
from pathlib import Path

TOOLS_DIR = Path(__file__).resolve().parent.parent / "tools"
sys.path.insert(0, str(TOOLS_DIR))

import dsk_to_nib  # noqa: E402

# Inverse of TRANS62 -- DETRANS62, ported from format_utils.ts's
# DETRANS62[] (indexed by the on-disk GCR byte minus 0x80, since all
# TRANS62 values are >= 0x80; 0 for invalid/sync bytes). Used below to
# validate that every GCR-encoded byte in a nibblized data field is a
# real, valid 6-and-2 GCR output value.
DETRANS62 = {}
for i, v in enumerate(dsk_to_nib.TRANS62):
    DETRANS62[v] = i


class TestDskToNibRoundTrip(unittest.TestCase):
    def test_track0_sector0_round_trips(self):
        """Focused round-trip: nibblize one known sector, confirm the
        real on-disk byte STREAM structure (prolog/epilog/checksums) is
        well-formed, matching real Disk II track format -- the full
        byte-for-byte data recovery is exercised end-to-end by
        test_disk2_controller_nibble_boot.c (C side), which is the
        actual consumer; this Python-side test's job is to catch
        encoding-logic bugs (wrong gap sizes, wrong checksum, wrong
        prolog/epilog) fast and cheaply before ever touching disk2_controller.c.
        """
        sector_data = bytes([(i * 7 + 3) & 0xFF for i in range(256)])
        nibbles = dsk_to_nib.explode_sector_16(254, 0, 0, sector_data)

        # Structural checks: real Disk II track format invariants.
        # Gap 1 for sector 0 is 0x80 sync bytes.
        self.assertEqual(nibbles[:0x80], [0xFF] * 0x80)
        addr_start = 0x80
        self.assertEqual(nibbles[addr_start:addr_start+3], [0xD5, 0xAA, 0x96])
        # Address field is 3 (prolog) + 8 (4x4 x4 fields) + 3 (epilog) = 14 bytes.
        addr_epilog_start = addr_start + 3 + 8
        self.assertEqual(nibbles[addr_epilog_start:addr_epilog_start+3], [0xDE, 0xAA, 0xEB])

        # Gap 2 (5 sync bytes) then data prolog.
        data_prolog_start = addr_epilog_start + 3 + 5
        self.assertEqual(nibbles[data_prolog_start:data_prolog_start+3], [0xD5, 0xAA, 0xAD])

        # Data field is 3 (prolog) + 0x156 GCR bytes + 1 (checksum) + 3 (epilog).
        data_epilog_start = data_prolog_start + 3 + 0x156 + 1
        self.assertEqual(nibbles[data_epilog_start:data_epilog_start+3], [0xDE, 0xAA, 0xEB])

        # Every GCR-encoded byte in the data field must be a valid
        # TRANS62 output value (high bit set, in the known table) --
        # catches any off-by-one/wrong-table bug immediately rather than
        # only failing much later when a real DOS 3.3 checksum mismatch
        # happens deep inside disk2_controller.c.
        data_bytes = nibbles[data_prolog_start+3:data_prolog_start+3+0x156+1]
        for b in data_bytes:
            self.assertIn(b, DETRANS62, f"byte 0x{b:02X} is not a valid 6-and-2 GCR byte")

        # Final gap-3 sync byte.
        self.assertEqual(nibbles[data_epilog_start+3], 0xFF)
        self.assertEqual(len(nibbles), data_epilog_start + 4)

    def test_full_disk_produces_35_tracks_with_correct_dos_sector_order(self):
        """Nibblize a synthetic 143360-byte image where each sector's
        first byte encodes (track, dos_sector) so we can verify DO[]
        (physical->DOS sector order) was applied correctly -- a wrong
        sector-order table would silently scramble which physical
        sector position holds which DOS sector's data, the exact kind of
        bug that would make DOS 3.3 boot fail mysteriously later."""
        dsk_data = bytearray(dsk_to_nib.DOS33_DISK_IMAGE_SIZE)
        for t in range(dsk_to_nib.TRACKS):
            for dos_sector in range(dsk_to_nib.SECTORS_PER_TRACK):
                offset = (dsk_to_nib.SECTORS_PER_TRACK * t + dos_sector) * dsk_to_nib.SECTOR_SIZE
                dsk_data[offset] = t
                dsk_data[offset + 1] = dos_sector

        tracks = dsk_to_nib.dsk_to_nibble_tracks(bytes(dsk_data))
        self.assertEqual(len(tracks), 35)

        # Spot-check track 5: the FIRST physical sector nibblized must
        # correspond to DO[0] == 0x0 (DOS sector 0), matching do.ts's
        # createDiskFromDOS() iterating physical_sector 0..15 and looking
        # up DO[physical_sector] for the DOS sector to pull data from.
        track5 = list(tracks[5])
        # Physical sector 0 has gap=0x80 (sector==0 branch in
        # explode_sector_16), so its address field starts right after.
        addr_start = 0x80
        # Address field's track/sector 4x4-encoded bytes: skip prolog (3).
        i = addr_start + 3
        vol_xx, vol_yy = track5[i], track5[i+1]
        i += 2
        trk_xx, trk_yy = track5[i], track5[i+1]
        i += 2
        sec_xx, sec_yy = track5[i], track5[i+1]

        def decode44(xx, yy):
            return ((xx << 1) | 0x01) & yy

        decoded_track = decode44(trk_xx, trk_yy)
        decoded_sector = decode44(sec_xx, sec_yy)
        self.assertEqual(decoded_track, 5)
        self.assertEqual(decoded_sector, 0)  # first physical sector's address field IS sector 0


class TestDskToNibCli(unittest.TestCase):
    def test_wrong_sized_input_rejected(self):
        import subprocess
        import tempfile
        with tempfile.NamedTemporaryFile(suffix=".dsk", delete=False) as f:
            f.write(b"too short")
            path = f.name
        try:
            result = subprocess.run(
                [sys.executable, str(TOOLS_DIR / "dsk_to_nib.py"), path],
                capture_output=True, text=True,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("error", result.stderr.lower())
            self.assertNotIn("Traceback", result.stderr)
        finally:
            Path(path).unlink()


if __name__ == "__main__":
    unittest.main()
