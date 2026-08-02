#include "woz_disk.h"
#include <string.h>

static uint32_t read_u32_le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t read_u16_le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

int woz_parse_image(const uint8_t *buffer, size_t buffer_size, woz_disk_t *out_disk) {
    if (!buffer || buffer_size < 12 || !out_disk) {
        return -1;
    }

    memset(out_disk, 0, sizeof(*out_disk));
    memset(out_disk->tmap, 0xFF, sizeof(out_disk->tmap));

    uint32_t magic = read_u32_le(buffer);
    if (magic != WOZ_MAGIC_WOZ1 && magic != WOZ_MAGIC_WOZ2) {
        return -1;
    }

    if (buffer[4] != 0xFF || buffer[5] != 0x0D || buffer[6] != 0x0A || buffer[7] != 0x7F) {
        return -1;
    }

    out_disk->version = (magic == WOZ_MAGIC_WOZ2) ? 2 : 1;

    size_t offset = 12;
    while (offset + 8 <= buffer_size) {
        uint32_t chunk_id = read_u32_le(&buffer[offset]);
        uint32_t chunk_size = read_u32_le(&buffer[offset + 4]);
        offset += 8;

        if (offset + chunk_size > buffer_size) {
            break; /* truncated chunk payload */
        }

        const uint8_t *payload = &buffer[offset];

        /* Chunk ID "INFO" (0x4F464E49) */
        if (chunk_id == 0x4F464E49u) {
            if (chunk_size >= 5) {
                out_disk->version = payload[0];
                out_disk->disk_type = payload[1];
                out_disk->write_protected = payload[2];
                out_disk->synchronized = payload[3];
                out_disk->cleaned_read = payload[4];
            }
        }
        /* Chunk ID "TMAP" (0x50414D54) */
        else if (chunk_id == 0x50414D54u) {
            size_t copy_len = chunk_size < WOZ_MAX_TRACKS ? chunk_size : WOZ_MAX_TRACKS;
            memcpy(out_disk->tmap, payload, copy_len);
        }
        /* Chunk ID "TRKS" (0x534B5254) */
        else if (chunk_id == 0x534B5254u) {
            if (out_disk->version == 1) {
                /* WOZ 1.0 TRKS format: 160 track entries x 6656 bytes */
                for (int t = 0; t < WOZ_MAX_TRACKS; t++) {
                    size_t track_offset = (size_t)t * 6656;
                    if (track_offset + 6656 > chunk_size) break;

                    const uint8_t *trk_data = &payload[track_offset];
                    uint16_t bytes_used = read_u16_le(&trk_data[6400]);
                    uint32_t bits = read_u32_le(&trk_data[6402]);

                    out_disk->track_bits[t] = trk_data;
                    out_disk->track_byte_count[t] = bytes_used;
                    out_disk->track_bit_count[t] = bits;
                }
            } else {
                /* WOZ 2.0 TRKS format: 160 track entries x 8 bytes header in payload */
                for (int t = 0; t < WOZ_MAX_TRACKS; t++) {
                    size_t entry_offset = (size_t)t * 8;
                    if (entry_offset + 8 > chunk_size) break;

                    const uint8_t *entry = &payload[entry_offset];
                    uint16_t start_block = read_u16_le(&entry[0]);
                    uint16_t block_count = read_u16_le(&entry[2]);
                    uint32_t bit_count = read_u32_le(&entry[4]);

                    if (start_block != 0 && bit_count != 0) {
                        size_t file_data_offset = (size_t)start_block * 512;
                        if (file_data_offset + ((bit_count + 7) / 8) <= buffer_size) {
                            out_disk->track_bits[t] = &buffer[file_data_offset];
                            out_disk->track_byte_count[t] = (uint16_t)((bit_count + 7) / 8);
                            out_disk->track_bit_count[t] = bit_count;
                        }
                    }
                }
            }
        }

        offset += chunk_size;
    }

    return 0;
}

uint8_t woz_read_bit(const woz_disk_t *disk, uint8_t quarter_track, uint32_t bit_index) {
    if (!disk || quarter_track >= WOZ_MAX_TRACKS) {
        return 0;
    }

    uint8_t trk_idx = disk->tmap[quarter_track];
    if (trk_idx == 0xFF || trk_idx >= WOZ_MAX_TRACKS) {
        return 0; /* unmapped track */
    }

    uint32_t total_bits = disk->track_bit_count[trk_idx];
    if (total_bits == 0 || bit_index >= total_bits) {
        return 0;
    }

    const uint8_t *bits = disk->track_bits[trk_idx];
    if (!bits) {
        return 0;
    }

    uint32_t byte_idx = bit_index / 8;
    uint8_t bit_pos = 7 - (bit_index % 8); /* MSB-first bit order */

    return (uint8_t)((bits[byte_idx] >> bit_pos) & 1);
}
