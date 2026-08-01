/*
 * disk_trap.c -- $C0E0-$C0EF fast-sector-read trap implementation.
 *
 * See disk_trap.h for the design rationale and mock-bus test harness note.
 * Minimal implementation to satisfy tests/test_disk_trap.c.
 */
#include "disk_trap.h"

static const uint8_t *g_disk_image = 0;
static uint32_t g_selected_sector_offset = 0;
static int g_have_selection = 0;

void disk_trap_set_image(const uint8_t *image) {
    g_disk_image = image;
}

void disk_trap_clear_image(void) {
    g_disk_image = (const uint8_t *)0;
    disk_trap_reset();
}

void disk_trap_reset(void) {
    g_selected_sector_offset = 0;
    g_have_selection = 0;
}

int disk_trap_has_selection(void) {
    return g_have_selection;
}

uint32_t disk_trap_get_selected_offset(void) {
    return g_selected_sector_offset;
}

int disk_trap_select_sector(uint8_t track, uint8_t sector) {
    uint32_t offset;
    if (dos33_sector_offset(track, sector, &offset) != 0) {
        /* Reject silently: leave any previous valid selection untouched
         * so a bad (track,sector) request can never corrupt an in-flight
         * read. */
        return -1;
    }
    g_selected_sector_offset = offset;
    g_have_selection = 1;
    return 0;
}

uint8_t disk_trap_read_byte(uint8_t byte_offset) {
    if (!g_have_selection || g_disk_image == 0) {
        return 0x00;
    }
    return g_disk_image[g_selected_sector_offset + byte_offset];
}
