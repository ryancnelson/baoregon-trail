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
