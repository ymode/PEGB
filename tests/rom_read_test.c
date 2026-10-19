#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Match the ROM directory emitted by tools/convert_rom.py. */
typedef struct {
    const char *title;
    const uint8_t *data;
    size_t size;
} rom_entry_t;

static const uint8_t short_rom[] = {0x00, 0x01, 0x7F, 0x80, 0xFE, 0xFF, 0x42};
static const uint8_t single_byte_rom[] = {0xA5};
static uint8_t banked_rom[0x10001];
static const rom_entry_t rom_list[] = {
    {"short", short_rom, sizeof(short_rom)},
    {"single byte", single_byte_rom, sizeof(single_byte_rom)},
    {"banked", banked_rom, sizeof(banked_rom)},
    {"empty null", NULL, 0},
    {"empty non-null", short_rom, 0},
    /* Accessible backing bytes must still be hidden by the declared size. */
    {"shortened", short_rom, 3},
};

struct gb_s;
static int current_rom = 0;

/* Generated at test time from the real callback, not a copy of its logic. */
#include "gb_rom_read.inc"

static size_t checks;

static void expect_byte(uint_fast32_t addr, uint8_t expected) {
    const uint8_t actual = gb_rom_read(NULL, addr);
    ++checks;
    if (actual != expected) {
        fprintf(stderr, "%s: address 0x%" PRIXFAST32
                " returned 0x%02X, expected 0x%02X\n",
                rom_list[current_rom].title, addr,
                (unsigned)actual, (unsigned)expected);
        exit(EXIT_FAILURE);
    }
}

static void test_valid_reads(void) {
    const int nonempty_roms[] = {0, 1, 2, 5};
    for (size_t i = 0; i < sizeof(banked_rom); ++i)
        banked_rom[i] = (uint8_t)((i * 37u + i / 256u) & 0xFFu);

    for (size_t i = 0; i < sizeof(nonempty_roms) / sizeof(nonempty_roms[0]); ++i) {
        current_rom = nonempty_roms[i];
        const rom_entry_t *rom = &rom_list[current_rom];
        /* Includes byte zero, interior bytes, and the last valid byte. */
        for (uint_fast32_t addr = 0; addr < rom->size; ++addr)
            expect_byte(addr, rom->data[addr]);
    }

    /* Switching back must use the selected ROM's data. */
    current_rom = 1;
    expect_byte(0, 0xA5);
    current_rom = 0;
    expect_byte(0, 0x00);
    puts("PASS: all valid bytes, last bytes, bank boundaries, and ROM switching");
}

static void test_boundaries(void) {
    const int nonempty_roms[] = {0, 1, 2, 5};
    for (size_t i = 0; i < sizeof(nonempty_roms) / sizeof(nonempty_roms[0]); ++i) {
        current_rom = nonempty_roms[i];
        const uint_fast32_t size = rom_list[current_rom].size;
        expect_byte(size, 0xFF);
        expect_byte(size + 1, 0xFF);
        expect_byte(size + 0x4000, 0xFF);
        expect_byte(UINT32_MAX, 0xFF);
        expect_byte(UINT_FAST32_MAX, 0xFF);
    }

    /* The same address changes validity when a different ROM is selected. */
    current_rom = 0;
    expect_byte(1, 0x01);
    current_rom = 1;
    expect_byte(1, 0xFF);
    current_rom = 0;
    expect_byte(1, 0x01);
    puts("PASS: exact/end+1/far boundaries, maximum addresses, and selected size");
}

static void test_empty_roms(void) {
    const uint_fast32_t addresses[] = {0, 1, 0x4000, UINT32_MAX, UINT_FAST32_MAX};
    for (int rom = 3; rom <= 4; ++rom) {
        current_rom = rom;
        for (size_t i = 0; i < sizeof(addresses) / sizeof(addresses[0]); ++i)
            expect_byte(addresses[i], 0xFF);
    }
    puts("PASS: zero-length ROMs with null and non-null data");
}

int main(int argc, char **argv) {
    const bool valid_only = argc == 2 && strcmp(argv[1], "--valid-only") == 0;
    if (argc > 1 && !valid_only) {
        fprintf(stderr, "Usage: %s [--valid-only]\n", argv[0]);
        return EXIT_FAILURE;
    }
    setvbuf(stdout, NULL, _IONBF, 0);
    test_valid_reads();
    if (!valid_only) {
        test_boundaries();
        test_empty_roms();
    }
    printf("PASS: %zu byte checks (uint_fast32_t: %zu bits)\n",
           checks, sizeof(uint_fast32_t) * 8);
    return EXIT_SUCCESS;
}
