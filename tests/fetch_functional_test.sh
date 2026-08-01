#!/bin/sh
# Fetches Klaus Dormann's 6502 functional test binary if not already present.
# The binary is gitignored (*.bin) -- this script is the source of truth for
# obtaining it, so a fresh checkout can still run `make test-functional`.
set -e

DEST="tests/fixtures/6502_functional_test.bin"
URL="https://raw.githubusercontent.com/Klaus2m5/6502_65C02_functional_tests/master/bin_files/6502_functional_test.bin"

mkdir -p tests/fixtures

if [ -f "$DEST" ]; then
    echo "Already present: $DEST"
    exit 0
fi

echo "Fetching Klaus Dormann's 6502 functional test binary..."
curl -sL --max-time 30 -o "$DEST" "$URL"

SIZE=$(wc -c < "$DEST" | tr -d ' ')
if [ "$SIZE" != "65536" ]; then
    echo "ERROR: expected 65536 bytes, got $SIZE bytes. Download likely failed." >&2
    rm -f "$DEST"
    exit 1
fi

echo "OK: $DEST ($SIZE bytes)"
