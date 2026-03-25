/**
 * Deck Window - Visual Card Selection UI
 *
 * Stub implementation. Full SDL2 UI will be added once the game
 * is running and we understand the card data format.
 *
 * For now, provides keyboard-based card selection as a fallback:
 *   F1-F12  = Select beetle cards 1-12
 *   F       = Next page of cards
 *   E       = Eject current card
 */

#include "deck_window.h"
#include "card_reader.h"
#include <stdio.h>

static int g_initialized = 0;
static int g_current_page = 0;
#define CARDS_PER_PAGE 12

int deck_window_init(void) {
    g_initialized = 1;
    printf("[DECK] Card deck initialized (keyboard mode)\n");
    printf("[DECK] Keys: F1-F12 = select card, F = next page, E = eject\n");
    return 0;
}

void deck_window_destroy(void) {
    g_initialized = 0;
}

void deck_window_update(NaomiIO *io) {
    (void)io;
    /* Keyboard-based card selection is handled in platform_poll_events.
     * A full SDL2 deck window will be implemented later. */
}

int deck_window_is_open(void) {
    return g_initialized;
}
