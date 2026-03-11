#ifndef MENU_H
#define MENU_H

#include <stdint.h>

// Show the game selection menu. Returns the selected ROM index.
// Renders to the provided 160x144 framebuffer, uses display_frame() and input_read().
int menu_select_game(uint16_t *framebuf, int rom_count, int current);

#endif // MENU_H
