#include <assert.h>
#include <stdio.h>
#include "input.h"
#include "hw_config.h"
#define ROM_DATA_IMPLEMENTATION
#include "rom_data.h"
static unsigned physical;
static const unsigned pins[] = {BTN_A, BTN_B, BTN_C, BTN_X, BTN_Y, BTN_Z};
void gpio_init(unsigned pin) {(void)pin;}
void gpio_set_dir(unsigned pin, bool out) {(void)pin;(void)out;}
void gpio_pull_up(unsigned pin) {(void)pin;}
bool gpio_get(unsigned pin) {
    for (unsigned i = 0; i < 6; ++i) if (pin == pins[i]) return !(physical & (1u << i));
    assert(false); return true;
}
static const uint8_t *script;
static size_t script_len, script_pos;
static joypad_t menu_input_read(void) {
    assert(script_pos < script_len);
    joypad_t jp = {.byte = script[script_pos++]};
    return jp;
}
static unsigned displays;
void display_frame(const uint16_t *fb) {(void)fb; ++displays;}
void sleep_ms(uint32_t ms) {(void)ms;}
#define input_read menu_input_read
#include "../src/menu.c"
#undef input_read
static void run_menu(const uint8_t *values, size_t count, int expected) {
    static uint16_t fb[160*144];
    script = values; script_len = count; script_pos = 0; displays = 0;
    assert(menu_select_game(fb, 2, 0) == expected);
    assert(script_pos == script_len);
    assert(displays >= 2);
}
int main(void) {
    for (physical = 0; physical < 64; ++physical) {
        joypad_t jp = input_read();
        bool select = (physical & 3) == 3;
        bool start = (physical & 36) == 36;
        assert(!jp.bits.select == select);
        assert(!jp.bits.start == start);
        assert(!jp.bits.a == !!(physical & 8));
        assert(!jp.bits.b == !!(physical & 16));
        assert(input_menu_requested(jp) == (select && start));
    }
    // Opening chord and held A cannot auto-confirm; B cancellation is drained.
    const uint8_t cancel[] = {0xF3, 0xFE, 0xFF, 0xFF, 0xFD, 0xFD, 0xFF};
    run_menu(cancel, sizeof cancel, 0);
    const uint8_t confirm[] = {0xFF, 0x7F, 0xFF, 0xFE, 0xFE, 0xFF};
    run_menu(confirm, sizeof confirm, 1);
    puts("PASS: all physical button combinations, standalone Select, menu chord and release handling");
}
