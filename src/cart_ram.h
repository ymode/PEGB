#ifndef CART_RAM_H
#define CART_RAM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct gb_s;

/* Clear RAM and select the cartridge's capacity. Unsupported sizes disable RAM. */
bool cart_ram_init(size_t size);
uint8_t gb_cart_ram_read(struct gb_s *gb, uint_fast32_t addr);
void gb_cart_ram_write(struct gb_s *gb, uint_fast32_t addr, uint8_t val);

#endif
