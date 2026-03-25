/**
 * Mushiking Card Reader Emulation
 *
 * Bridges the deck window UI to the JVS card reader interface.
 * When a card is selected in the deck window, this module feeds
 * the card data to the Naomi JVS I/O as if a physical card was inserted.
 */

#ifndef CARD_READER_H
#define CARD_READER_H

#include "hal/naomi_io.h"

/* Initialize card reader with JVS I/O reference */
void card_reader_init(NaomiIO *io);

/* Destroy card reader */
void card_reader_destroy(void);

/* Insert a beetle card by ID */
void card_reader_insert_beetle(uint32_t beetle_id);

/* Eject current card */
void card_reader_eject(void);

/* Known beetle card IDs (extracted from ROM) */
typedef struct {
    uint32_t id;
    const char *name;
    const char *rom_name;  /* Internal ROM name */
} BeetleCardInfo;

/* Get list of all known beetle cards */
const BeetleCardInfo* card_reader_get_beetle_list(int *count);

#endif /* CARD_READER_H */
