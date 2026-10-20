#ifndef TEST_ROM_DATA_H
#define TEST_ROM_DATA_H
#include <stdint.h>
#include <stddef.h>
typedef struct { const char *title; const uint8_t *data; size_t size; } rom_entry_t;
#define ROM_COUNT 2
extern const rom_entry_t rom_list[ROM_COUNT];
#ifdef ROM_DATA_IMPLEMENTATION
// Two synthetic ROM-only cartridges; valid header checksum and endless JP loop.
static const uint8_t synthetic_rom[32768] = {[0x100]=0xC3, [0x102]=1, [0x14D]=0xE7};
const rom_entry_t rom_list[ROM_COUNT] = {
    {"ONE", synthetic_rom, sizeof synthetic_rom},
    {"TWO", synthetic_rom, sizeof synthetic_rom}
};
#endif
#endif
