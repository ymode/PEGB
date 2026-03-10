#include "input.h"
#include "hw_config.h"
#include "pico/stdlib.h"

/*
 * Button mapping:
 *   Explorer  →  Game Boy
 *   A            D-pad Up
 *   B            D-pad Down
 *   C            D-pad Left
 *   Z            D-pad Right
 *   X            A button
 *   Y            B button
 *   C + Z        Start  (Left+Right combo — impossible on real d-pad)
 *   A + B        Select (Up+Down combo — impossible on real d-pad)
 *
 * Buttons are active low (pulled up, read 0 when pressed).
 */

static const uint btn_pins[] = { BTN_A, BTN_B, BTN_C, BTN_X, BTN_Y, BTN_Z };

void input_init(void) {
    for (int i = 0; i < 6; i++) {
        gpio_init(btn_pins[i]);
        gpio_set_dir(btn_pins[i], GPIO_IN);
        gpio_pull_up(btn_pins[i]);
    }
}

joypad_t input_read(void) {
    bool a_pressed = !gpio_get(BTN_A);
    bool b_pressed = !gpio_get(BTN_B);
    bool c_pressed = !gpio_get(BTN_C);
    bool x_pressed = !gpio_get(BTN_X);
    bool y_pressed = !gpio_get(BTN_Y);
    bool z_pressed = !gpio_get(BTN_Z);

    joypad_t jp;
    jp.byte = 0xFF;  // All released

    // Combo detection: opposite d-pad directions → meta buttons
    bool combo_start  = c_pressed && z_pressed;  // Left + Right
    bool combo_select = a_pressed && b_pressed;   // Up + Down

    if (combo_start)  jp.bits.start  = 0;
    if (combo_select) jp.bits.select = 0;

    // D-pad (suppress individual directions when combo is active)
    if (a_pressed && !combo_select) jp.bits.up    = 0;
    if (b_pressed && !combo_select) jp.bits.down  = 0;
    if (c_pressed && !combo_start)  jp.bits.left  = 0;
    if (z_pressed && !combo_start)  jp.bits.right = 0;

    // Action buttons
    if (x_pressed) jp.bits.a = 0;
    if (y_pressed) jp.bits.b = 0;

    return jp;
}
