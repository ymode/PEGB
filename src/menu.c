#include "menu.h"
#include "font.h"
#include "display.h"
#include "input.h"
#include "rom_data.h"
#include "pico/stdlib.h"

// GB native resolution
#define SCR_W 160
#define SCR_H 144

// Layout
#define TITLE_Y     2    // "SELECT GAME" header row (in pixels)
#define LIST_Y      20   // First game entry Y
#define LINE_H      12   // Pixels per entry
#define MAX_VISIBLE ((SCR_H - LIST_Y) / LINE_H)

// Colors (native RGB565)
#define COL_BG      0x0000  // Black
#define COL_TEXT    0xFFFF  // White
#define COL_CURSOR  0x07E0  // Green
#define COL_HEADER  0xFFE0  // Yellow

static void draw_char(uint16_t *fb, int x, int y, char c, uint16_t color) {
    const uint8_t *glyph = font_get_glyph(c);
    for (int row = 0; row < FONT_CHAR_H && (y + row) < SCR_H; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < FONT_CHAR_W && (x + col) < SCR_W; col++) {
            if (bits & (0x80 >> col)) {
                fb[(y + row) * SCR_W + (x + col)] = color;
            }
        }
    }
}

static void draw_string(uint16_t *fb, int x, int y, const char *str, uint16_t color) {
    while (*str && x < SCR_W) {
        draw_char(fb, x, y, *str, color);
        x += FONT_CHAR_W;
        str++;
    }
}

static void draw_menu(uint16_t *fb, int cursor, int scroll, int rom_count) {
    // Clear framebuffer
    for (int i = 0; i < SCR_W * SCR_H; i++)
        fb[i] = COL_BG;

    // Header
    draw_string(fb, 28, TITLE_Y, "SELECT GAME", COL_HEADER);

    // Game list
    int visible = rom_count < (int)MAX_VISIBLE ? rom_count : (int)MAX_VISIBLE;
    for (int i = 0; i < visible; i++) {
        int rom_idx = scroll + i;
        if (rom_idx >= rom_count) break;

        int y = LIST_Y + i * LINE_H;
        uint16_t color = (rom_idx == cursor) ? COL_CURSOR : COL_TEXT;

        // Cursor arrow
        if (rom_idx == cursor) {
            draw_char(fb, 2, y, '>', color);
        }

        // ROM title (truncated to fit)
        const char *title = rom_list[rom_idx].title;
        draw_string(fb, 14, y, title, color);
    }

    // Scroll indicators
    if (scroll > 0) {
        draw_char(fb, SCR_W - 10, LIST_Y - 2, '^', COL_TEXT);
    }
    if (scroll + (int)MAX_VISIBLE < rom_count) {
        draw_char(fb, SCR_W - 10, LIST_Y + visible * LINE_H, 'v', COL_TEXT);
    }
}

int menu_select_game(uint16_t *framebuf, int rom_count, int current) {
    if (rom_count <= 0) return 0;
    if (rom_count == 1) return 0;

    int cursor = current;
    int scroll = 0;

    // Adjust scroll to keep cursor visible
    if (cursor >= scroll + (int)MAX_VISIBLE)
        scroll = cursor - (int)MAX_VISIBLE + 1;

    // Wait for Select to be released before entering menu
    while (1) {
        joypad_t jp = input_read();
        if (jp.bits.select) break;  // select=1 means released (active low)
        sleep_ms(10);
    }

    uint8_t prev_byte = 0xFF; // All released

    while (1) {
        draw_menu(framebuf, cursor, scroll, rom_count);
        display_frame(framebuf);

        sleep_ms(50); // Simple debounce

        joypad_t jp = input_read();

        // Detect fresh presses (was released, now pressed)
        // Active low: 0 = pressed, 1 = released
        uint8_t pressed = prev_byte & ~jp.byte;
        prev_byte = jp.byte;

        // Up (bit 6)
        if (pressed & (1 << 6)) {
            if (cursor > 0) {
                cursor--;
                if (cursor < scroll) scroll = cursor;
            }
        }
        // Down (bit 7)
        if (pressed & (1 << 7)) {
            if (cursor < rom_count - 1) {
                cursor++;
                if (cursor >= scroll + (int)MAX_VISIBLE)
                    scroll = cursor - (int)MAX_VISIBLE + 1;
            }
        }
        // A button (bit 0) = confirm
        if (pressed & (1 << 0)) {
            // Wait for release
            while (1) {
                jp = input_read();
                if (jp.bits.a) break;
                sleep_ms(10);
            }
            return cursor;
        }
        // B button (bit 1) = cancel, return current
        if (pressed & (1 << 1)) {
            return current;
        }
    }
}
