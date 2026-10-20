#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "cart_ram.h"

#ifdef TEST_PEANUT_GB
#include "peanut_gb.h"

static uint8_t rom[32768];
static uint8_t read_rom(struct gb_s *gb, uint_fast32_t addr) {
    (void)gb;
    return addr < sizeof(rom) ? rom[addr] : 0xFF;
}
static void core_error(struct gb_s *gb, enum gb_error_e err, uint16_t addr) {
    (void)gb;
    fprintf(stderr, "Core error %d at %04x\n", err, addr);
    exit(EXIT_FAILURE);
}
#endif

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static uint8_t pattern(uint_fast32_t addr) {
    return (uint8_t)(addr * 37u + addr / 8192u);
}

static void test_capacity(size_t size) {
    CHECK(cart_ram_init(size));
    for (uint_fast32_t addr = 0; addr < size; ++addr) {
        CHECK(gb_cart_ram_read(NULL, addr) == 0);
        gb_cart_ram_write(NULL, addr, pattern(addr));
    }
    const uint_fast32_t invalid[] = {
        size, size + 1, 128 * 1024, 128 * 1024 + 1,
        UINT32_MAX, UINT_FAST32_MAX
    };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        gb_cart_ram_write(NULL, invalid[i], 0xEE);
        CHECK(gb_cart_ram_read(NULL, invalid[i]) == 0xFF);
    }
    // Recheck every byte to catch bank aliasing or invalid writes wrapping.
    for (uint_fast32_t addr = 0; addr < size; ++addr)
        CHECK(gb_cart_ram_read(NULL, addr) == pattern(addr));
    printf("PASS: %zu-byte RAM, all bytes, bank separation and boundaries\n", size);
}

static void test_switch(void) {
    CHECK(cart_ram_init(128 * 1024));
    gb_cart_ram_write(NULL, 0, 0x55);
    gb_cart_ram_write(NULL, 128 * 1024 - 1, 0xAA);
    CHECK(cart_ram_init(512));
    CHECK(gb_cart_ram_read(NULL, 0) == 0);
    CHECK(gb_cart_ram_read(NULL, 128 * 1024 - 1) == 0xFF);
    CHECK(cart_ram_init(128 * 1024));
    CHECK(gb_cart_ram_read(NULL, 128 * 1024 - 1) == 0);
    CHECK(!cart_ram_init(128 * 1024 + 1));
    gb_cart_ram_write(NULL, 0, 0xAA);
    CHECK(gb_cart_ram_read(NULL, 0) == 0xFF);
    CHECK(!cart_ram_init(SIZE_MAX));
    CHECK(gb_cart_ram_read(NULL, 0) == 0xFF);
    CHECK(cart_ram_init(0));
    CHECK(gb_cart_ram_read(NULL, 0) == 0xFF);
    puts("PASS: cartridge switches clear RAM; rejected/zero sizes disable access");
}

#ifdef TEST_PEANUT_GB
static void test_mapper(uint8_t size_code, size_t expected_size) {
    struct gb_s gb = {0};
    memset(rom, 0, sizeof(rom));
    rom[0x147] = 0x1B; // MBC5 + RAM + battery
    rom[0x149] = size_code;
    for (unsigned i = 0x134; i <= 0x14C; ++i)
        rom[0x14D] = (uint8_t)(rom[0x14D] - rom[i] - 1);
    CHECK(cart_ram_init(0));
    CHECK(gb_init(&gb, read_rom, gb_cart_ram_read, gb_cart_ram_write,
                  core_error, NULL) == GB_INIT_NO_ERROR);
    size_t size = 0;
    CHECK(gb_get_save_size_s(&gb, &size) == 0);
    CHECK(size == expected_size);
    CHECK(cart_ram_init(size));
    __gb_write(&gb, 0x0000, 0x0A); // enable RAM
    const unsigned banks = (unsigned)(size / 8192);
    for (unsigned bank = 0; bank < banks; ++bank) {
        __gb_write(&gb, 0x4000, (uint8_t)bank);
        for (unsigned offset = 0; offset < 8192; ++offset)
            __gb_write(&gb, (uint16_t)(0xA000 + offset),
                       pattern(bank * 8192 + offset));
    }
    for (unsigned bank = 0; bank < banks; ++bank) {
        __gb_write(&gb, 0x4000, (uint8_t)bank);
        for (unsigned offset = 0; offset < 8192; ++offset)
            CHECK(__gb_read(&gb, (uint16_t)(0xA000 + offset)) ==
                  pattern(bank * 8192 + offset));
    }
    printf("PASS: actual Peanut-GB MBC5 bus accesses, %u distinct RAM banks\n", banks);
}
#endif

int main(void) {
    const size_t sizes[] = {0, 512, 2048, 8192, 32768, 65536, 131072};
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i)
        test_capacity(sizes[i]);
    test_switch();
#ifdef TEST_PEANUT_GB
    test_mapper(3, 32768);
    test_mapper(5, 65536);
    test_mapper(4, 131072);
#endif
    return 0;
}
