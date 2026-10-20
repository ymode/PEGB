#include "cart_ram.h"

#include <string.h>

/* Largest RAM size supported by Peanut-GB (MBC5: 16 banks of 8 KiB). */
static uint8_t cart_ram[128 * 1024];
static size_t cart_ram_size;

bool cart_ram_init(size_t size) {
    cart_ram_size = 0;
    if (size > sizeof(cart_ram))
        return false;

    memset(cart_ram, 0, sizeof(cart_ram));
    cart_ram_size = size;
    return true;
}

uint8_t gb_cart_ram_read(struct gb_s *gb, const uint_fast32_t addr) {
    (void)gb;
    if (addr >= cart_ram_size)
        return 0xFF;
    return cart_ram[addr];
}

void gb_cart_ram_write(struct gb_s *gb, const uint_fast32_t addr, const uint8_t val) {
    (void)gb;
    if (addr < cart_ram_size)
        cart_ram[addr] = val;
}
