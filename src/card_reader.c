/**
 * Mushiking Card Reader Emulation
 */

#include "card_reader.h"
#include <stdio.h>
#include <string.h>

static NaomiIO *g_io = NULL;

/* Known beetle cards from ROM string table.
 * IDs are preliminary - will be updated from ROM data analysis. */
static const BeetleCardInfo g_beetles[] = {
    { 0x0001, "Caucasus Beetle",        "CAUCASUS" },
    { 0x0002, "Hercules Beetle",        "HERCULES" },
    { 0x0003, "Hercules Blue",          "HERCULESBLUE" },
    { 0x0004, "Giraffe Stag",           "GIRAFFA" },
    { 0x0005, "Grandis Stag",           "GRANDIS" },
    { 0x0006, "Five-Horned Beetle",     "GOHONDUNO" },
    { 0x0007, "Atlas Beetle",           "ALCIDESHIRATA" },
    { 0x0008, "Centaurus Beetle",       "CENTAUKAB" },
    { 0x0009, "Didier's Stag",          "DIDIERKUWA" },
    { 0x000A, "Fabricius' Stag",        "FABRICEKUWA" },
    { 0x000B, "Feisthamel's Stag",      "FEISTHAKUWA" },
    { 0x000C, "Haster's Stag",          "HASTERTKUWA" },
    { 0x000D, "Imperial Stag",          "IMPERIKUWA" },
    { 0x000E, "Burmese Stag",           "BURMEIKUWA" },
    { 0x000F, "Japanese Rhino Beetle",  "KABUTO" },
    { 0x0010, "Hime Beetle",            "HIMEKAB" },
    { 0x0011, "Hime Beetle SH",         "HIMEKAB_SH" },
    { 0x0012, "Hirata's Stag",          "HIRATA" },
    { 0x0013, "Hirata's Stag SH",       "HIRATA_SH" },
    { 0x0014, "Hosoaka",                "HOSOAKA" },
    { 0x0015, "Firefly",                "HOTARU" },
    { 0x0016, "Blue Swallowtail",       "AOSUJIAGEHA" },
    { 0x0017, "Anubis Beetle",          "ANUBISZOUKAB" },
    { 0x0018, "Heraduno Beetle",        "HERADUNOKAB" },
};

#define NUM_BEETLES (sizeof(g_beetles) / sizeof(g_beetles[0]))

void card_reader_init(NaomiIO *io) {
    g_io = io;
    printf("[CARD] Card reader initialized (%zu beetles in database)\n", NUM_BEETLES);
}

void card_reader_destroy(void) {
    if (g_io) {
        naomi_io_eject_card(g_io);
    }
    g_io = NULL;
}

void card_reader_insert_beetle(uint32_t beetle_id) {
    if (!g_io) return;

    /* Find beetle info */
    const BeetleCardInfo *info = NULL;
    for (size_t i = 0; i < NUM_BEETLES; i++) {
        if (g_beetles[i].id == beetle_id) {
            info = &g_beetles[i];
            break;
        }
    }

    if (!info) {
        printf("[CARD] Unknown beetle ID: 0x%04X\n", beetle_id);
        return;
    }

    /* Build card data packet.
     * The actual card data format will need to be reverse-engineered
     * from the game code. For now, use a simple format with the
     * beetle's ROM name as the card identifier. */
    uint8_t card_data[JVS_CARD_DATA_SIZE];
    memset(card_data, 0, sizeof(card_data));

    /* Card header */
    card_data[0] = 'M';  /* Mushiking card marker */
    card_data[1] = 'K';
    card_data[2] = (uint8_t)(beetle_id >> 8);
    card_data[3] = (uint8_t)(beetle_id & 0xFF);

    /* Copy ROM name */
    size_t name_len = strlen(info->rom_name);
    if (name_len > JVS_CARD_DATA_SIZE - 8) name_len = JVS_CARD_DATA_SIZE - 8;
    memcpy(card_data + 4, info->rom_name, name_len);

    naomi_io_insert_card(g_io, card_data, sizeof(card_data), beetle_id);
    printf("[CARD] Inserted: %s (%s)\n", info->name, info->rom_name);
}

void card_reader_eject(void) {
    if (!g_io) return;
    naomi_io_eject_card(g_io);
}

const BeetleCardInfo* card_reader_get_beetle_list(int *count) {
    if (count) *count = (int)NUM_BEETLES;
    return g_beetles;
}
