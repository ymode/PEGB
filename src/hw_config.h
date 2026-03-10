#ifndef HW_CONFIG_H
#define HW_CONFIG_H

/*
 * Pimoroni Explorer (RP2350B) pin assignments.
 *
 * Display: ST7789 320x240 via 8-bit parallel bus (PIO-driven)
 * These are from the Pimoroni pimoroni-pico-rp2350 source.
 *
 * NOTE: Button GPIOs below are best-effort from available docs.
 *       Verify against your board and update if needed.
 */

// --- Display (ST7789 parallel) ---
#define DISPLAY_CS      27
#define DISPLAY_DC      28
#define DISPLAY_WR      30
#define DISPLAY_RD      31
#define DISPLAY_D0      32   // D0-D7 = GPIO 32..39
#define DISPLAY_BL      26
#define DISPLAY_WIDTH   320
#define DISPLAY_HEIGHT  240

// --- Buttons ---
#define BTN_A   16
#define BTN_B   15
#define BTN_C   14
#define BTN_X   17
#define BTN_Y   18
#define BTN_Z   19

// --- Audio ---
#define PIN_AUDIO       12
#define PIN_AMP_EN      13

// --- Game Boy screen ---
#define GB_LCD_WIDTH    160
#define GB_LCD_HEIGHT   144

#endif // HW_CONFIG_H
