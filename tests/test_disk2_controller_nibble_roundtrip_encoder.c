/*
 * tests/test_disk2_controller_nibble_roundtrip_encoder.c -- standalone
 * C port of tools/dsk_to_nib.py's explode_sector_16(), used ONLY by
 * tests/test_disk2_controller_nibble_roundtrip.c as an independent
 * encode-side implementation (kept deliberately separate from
 * dsk_to_nib.py so the round-trip test isn't just re-running the same
 * Python code against itself via a subprocess).
 *
 * Ported from whscullin/apple2js (MIT License) -- see
 * src/disk2_controller.c's file header for the full MIT notice; this
 * file reproduces the same algorithm (format_utils.ts's
 * explodeSector16()) as tools/dsk_to_nib.py, just in C for this test.
 */
#include <stdint.h>

static const uint8_t TRANS62_FOR_TEST[64] = {
    0x96, 0x97, 0x9A, 0x9B, 0x9D, 0x9E, 0x9F, 0xA6,
    0xA7, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB2, 0xB3,
    0xB4, 0xB5, 0xB6, 0xB7, 0xB9, 0xBA, 0xBB, 0xBC,
    0xBD, 0xBE, 0xBF, 0xCB, 0xCD, 0xCE, 0xCF, 0xD3,
    0xD6, 0xD7, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE,
    0xDF, 0xE5, 0xE6, 0xE7, 0xE9, 0xEA, 0xEB, 0xEC,
    0xED, 0xEE, 0xEF, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6,
    0xF7, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
};

static void four_x_four_for_test(uint8_t val, uint8_t *xx, uint8_t *yy) {
    *xx = val & 0xAA;
    *yy = val & 0x55;
    *xx >>= 1;
    *xx |= 0xAA;
    *yy |= 0xAA;
}

void encode_sector_16_for_test(uint8_t volume, uint8_t track, uint8_t sector,
                                const uint8_t data[256], uint8_t *out, int *out_len) {
    int p = 0;

    int gap;
    if (sector == 0) {
        gap = 0x80;
    } else {
        gap = (track == 0) ? 0x28 : 0x26;
    }
    for (int i = 0; i < gap; i++) {
        out[p++] = 0xFF;
    }

    uint8_t checksum = (uint8_t)(volume ^ track ^ sector);
    out[p++] = 0xD5; out[p++] = 0xAA; out[p++] = 0x96;
    uint8_t xx, yy;
    four_x_four_for_test(volume, &xx, &yy); out[p++] = xx; out[p++] = yy;
    four_x_four_for_test(track, &xx, &yy); out[p++] = xx; out[p++] = yy;
    four_x_four_for_test(sector, &xx, &yy); out[p++] = xx; out[p++] = yy;
    four_x_four_for_test(checksum, &xx, &yy); out[p++] = xx; out[p++] = yy;
    out[p++] = 0xDE; out[p++] = 0xAA; out[p++] = 0xEB;

    for (int i = 0; i < 0x05; i++) {
        out[p++] = 0xFF;
    }

    out[p++] = 0xD5; out[p++] = 0xAA; out[p++] = 0xAD;

    static uint8_t nibbles[0x158];
    for (int i = 0; i < 0x158; i++) {
        nibbles[i] = 0;
    }
    int ptr2 = 0;
    int ptr6 = 0x56;
    int idx2 = 0x55;
    for (int idx6 = 0x101; idx6 >= 0; idx6--) {
        uint8_t val6 = data[idx6 % 0x100];
        uint8_t val2 = nibbles[ptr2 + idx2];

        val2 = (uint8_t)((val2 << 1) | (val6 & 1));
        val6 = (uint8_t)(val6 >> 1);
        val2 = (uint8_t)((val2 << 1) | (val6 & 1));
        val6 = (uint8_t)(val6 >> 1);

        nibbles[ptr6 + idx6] = val6;
        nibbles[ptr2 + idx2] = val2;

        idx2--;
        if (idx2 < 0) {
            idx2 = 0x55;
        }
    }

    uint8_t last = 0;
    for (int i = 0; i < 0x156; i++) {
        uint8_t val = nibbles[i];
        out[p++] = TRANS62_FOR_TEST[last ^ val];
        last = val;
    }
    out[p++] = TRANS62_FOR_TEST[last];

    out[p++] = 0xDE; out[p++] = 0xAA; out[p++] = 0xEB;

    out[p++] = 0xFF;

    *out_len = p;
}
