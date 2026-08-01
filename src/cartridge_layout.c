/*
 * cartridge_layout.c -- ReRAM game-slot table, see cartridge_layout.h.
 */
#include "cartridge_layout.h"

const cartridge_slot_t cartridge_slots[CARTRIDGE_SLOT_COUNT] = {
    { "The Oregon Trail (1985)",                  CARTRIDGE_RERAM_BASE + 0 * CARTRIDGE_SLOT_SIZE },
    { "Where in the World is Carmen Sandiego?",   CARTRIDGE_RERAM_BASE + 1 * CARTRIDGE_SLOT_SIZE },
    { "Karateka",                                 CARTRIDGE_RERAM_BASE + 2 * CARTRIDGE_SLOT_SIZE },
    { "Lode Runner",                              CARTRIDGE_RERAM_BASE + 3 * CARTRIDGE_SLOT_SIZE },
    { "Prince of Persia (Disk 1)",                CARTRIDGE_RERAM_BASE + 4 * CARTRIDGE_SLOT_SIZE },
    { "Ultima IV",                                CARTRIDGE_RERAM_BASE + 5 * CARTRIDGE_SLOT_SIZE },
};

const cartridge_slot_t *cartridge_layout_get_slot(int index) {
    if (index < 0 || index >= CARTRIDGE_SLOT_COUNT) {
        return 0;
    }
    return &cartridge_slots[index];
}

const cartridge_slot_t *cartridge_layout_find_slot_by_title(const char *title) {
    if (!title) {
        return 0;
    }
    for (int i = 0; i < CARTRIDGE_SLOT_COUNT; i++) {
        const char *t = cartridge_slots[i].title;
        if (t) {
            const char *p1 = title;
            const char *p2 = t;
            while (*p1 && (*p1 == *p2)) {
                p1++;
                p2++;
            }
            if (*p1 == *p2) {
                return &cartridge_slots[i];
            }
        }
    }
    return 0;
}
