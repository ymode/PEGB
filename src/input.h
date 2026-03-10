#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

// Joypad bitfield matching Peanut-GB's gb.direct.joypad layout.
// Each bit: 1 = released, 0 = pressed (active low).
typedef union {
    struct {
        uint8_t a      : 1;
        uint8_t b      : 1;
        uint8_t select : 1;
        uint8_t start  : 1;
        uint8_t right  : 1;
        uint8_t left   : 1;
        uint8_t up     : 1;
        uint8_t down   : 1;
    } bits;
    uint8_t byte;
} joypad_t;

void input_init(void);
joypad_t input_read(void);

#endif // INPUT_H
