#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include "hw_config.h"

// Pack an RGB565 value in big-endian byte order (ST7789 expects MSB first)
static inline uint16_t rgb565_be(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    return __builtin_bswap16(c);
}

void display_init(void);
void display_fill(uint16_t color);  // Fill entire screen with a color (for diagnostics)
void display_frame(const uint16_t *framebuf);

#endif // DISPLAY_H
