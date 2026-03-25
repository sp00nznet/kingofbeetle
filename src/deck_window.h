/**
 * Deck Window - Visual Card Selection UI
 *
 * Displays a separate window showing available beetle cards.
 * Click a card to "insert" it into the virtual card reader.
 * This simulates the physical card scanning in the arcade.
 */

#ifndef DECK_WINDOW_H
#define DECK_WINDOW_H

#include "hal/naomi_io.h"

/* Initialize the deck window (creates second SDL2 window if available) */
int deck_window_init(void);

/* Destroy the deck window */
void deck_window_destroy(void);

/* Update deck window - handle clicks, draw cards */
void deck_window_update(NaomiIO *io);

/* Check if deck window is open */
int deck_window_is_open(void);

#endif /* DECK_WINDOW_H */
