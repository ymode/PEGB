#include "display.h"
#include "hw_config.h"

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "display.pio.h"

static PIO pio;
static uint sm;
static int dma_chan;
static bool use_pio = false;

// --- Bit-bang helpers ---

static inline void bb_write_byte(uint8_t val) {
    for (int i = 0; i < 8; i++) {
        gpio_put(DISPLAY_D0 + i, (val >> i) & 1);
    }
    gpio_put(DISPLAY_WR, 0);
    __asm volatile("nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop");
    gpio_put(DISPLAY_WR, 1);
    __asm volatile("nop \n nop \n nop \n nop");
}

static void bb_cmd(uint8_t cmd) {
    gpio_put(DISPLAY_DC, 0);
    bb_write_byte(cmd);
}

static void bb_data8(uint8_t data) {
    gpio_put(DISPLAY_DC, 1);
    bb_write_byte(data);
}

// --- PIO helpers ---

static inline void st7789_wait_idle(void) {
    while (!pio_sm_is_tx_fifo_empty(pio, sm))
        tight_loop_contents();
    __asm volatile("nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop");
    __asm volatile("nop \n nop \n nop \n nop \n nop \n nop \n nop \n nop");
}

// --- Unified command/data ---

static void st7789_cmd(uint8_t cmd) {
    if (use_pio) {
        st7789_wait_idle();
        gpio_put(DISPLAY_DC, 0);
        pio_sm_put_blocking(pio, sm, cmd);
        st7789_wait_idle();
    } else {
        bb_cmd(cmd);
    }
}

static void st7789_data8(uint8_t data) {
    if (use_pio) {
        st7789_wait_idle();
        gpio_put(DISPLAY_DC, 1);
        pio_sm_put_blocking(pio, sm, data);
    } else {
        bb_data8(data);
    }
}

// --- ST7789 commands ---

static void st7789_init_seq(void) {
    st7789_cmd(0x01);
    sleep_ms(150);

    st7789_cmd(0x11);
    sleep_ms(50);

    st7789_cmd(0x3A);
    st7789_data8(0x55);

    st7789_cmd(0x36);
    st7789_data8(0x60);

    st7789_cmd(0x21);

    st7789_cmd(0x13);
    sleep_ms(10);

    st7789_cmd(0x29);
    sleep_ms(50);
}

static void st7789_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    st7789_cmd(0x2A);
    st7789_data8(x0 >> 8);
    st7789_data8(x0 & 0xFF);
    st7789_data8(x1 >> 8);
    st7789_data8(x1 & 0xFF);

    st7789_cmd(0x2B);
    st7789_data8(y0 >> 8);
    st7789_data8(y0 & 0xFF);
    st7789_data8(y1 >> 8);
    st7789_data8(y1 & 0xFF);

    st7789_cmd(0x2C);
}

// --- GPIO init ---

static void display_gpio_init(void) {
    gpio_init(DISPLAY_CS);
    gpio_set_dir(DISPLAY_CS, GPIO_OUT);
    gpio_put(DISPLAY_CS, 0);

    gpio_init(DISPLAY_DC);
    gpio_set_dir(DISPLAY_DC, GPIO_OUT);

    gpio_init(DISPLAY_WR);
    gpio_set_dir(DISPLAY_WR, GPIO_OUT);
    gpio_put(DISPLAY_WR, 1);

    gpio_init(DISPLAY_RD);
    gpio_set_dir(DISPLAY_RD, GPIO_OUT);
    gpio_put(DISPLAY_RD, 1);

    gpio_init(DISPLAY_BL);
    gpio_set_dir(DISPLAY_BL, GPIO_OUT);
    gpio_put(DISPLAY_BL, 1);

    for (int i = 0; i < 8; i++) {
        gpio_init(DISPLAY_D0 + i);
        gpio_set_dir(DISPLAY_D0 + i, GPIO_OUT);
        gpio_put(DISPLAY_D0 + i, 0);
    }
}

// --- PIO setup ---

static void display_pio_init(void) {
    uint offset;
    bool ok = pio_claim_free_sm_and_add_program_for_gpio_range(
        &st7789_parallel_program, &pio, &sm, &offset,
        DISPLAY_WR,
        DISPLAY_D0 + 8 - DISPLAY_WR,
        true
    );
    if (!ok) return;

    pio_sm_config c = st7789_parallel_program_get_default_config(offset);

    sm_config_set_out_pins(&c, DISPLAY_D0, 8);
    for (int i = 0; i < 8; i++) {
        pio_gpio_init(pio, DISPLAY_D0 + i);
    }
    pio_sm_set_consecutive_pindirs(pio, sm, DISPLAY_D0, 8, true);

    sm_config_set_sideset_pins(&c, DISPLAY_WR);
    pio_gpio_init(pio, DISPLAY_WR);
    pio_sm_set_consecutive_pindirs(pio, sm, DISPLAY_WR, 1, true);

    sm_config_set_out_shift(&c, true, true, 8);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    sm_config_set_clkdiv(&c, 8.0f);  // Conservative: 250/8 = 31.25 MHz

    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);

    dma_chan = dma_claim_unused_channel(true);
    use_pio = true;
}

// --- Public API ---

void display_init(void) {
    // Phase 1: bit-bang init + clear to black
    display_gpio_init();
    st7789_init_seq();

    st7789_set_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
    gpio_put(DISPLAY_DC, 1);
    for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
        bb_write_byte(0x00);
        bb_write_byte(0x00);
    }

    // Phase 2: switch to PIO
    display_pio_init();
    if (use_pio) {
        st7789_init_seq();
    }
}

void display_fill(uint16_t color) {
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    st7789_set_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
    if (use_pio) {
        st7789_wait_idle();
        gpio_put(DISPLAY_DC, 1);
        for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
            pio_sm_put_blocking(pio, sm, hi);
            pio_sm_put_blocking(pio, sm, lo);
        }
        st7789_wait_idle();
    } else {
        gpio_put(DISPLAY_DC, 1);
        for (int i = 0; i < DISPLAY_WIDTH * DISPLAY_HEIGHT; i++) {
            bb_write_byte(hi);
            bb_write_byte(lo);
        }
    }
}

void display_frame(const uint16_t *framebuf) {
    // Line buffer: 320 pixels * 2 bytes = 640 bytes, stored big-endian for ST7789
    static uint8_t line_buf[DISPLAY_WIDTH * 2];

    st7789_set_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);

    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        int src_y = y * GB_LCD_HEIGHT / DISPLAY_HEIGHT;
        const uint16_t *src = &framebuf[src_y * GB_LCD_WIDTH];

        // Fill line_buf with 2x scaled pixels in big-endian byte order
        uint8_t *dst = line_buf;
        for (int x = 0; x < GB_LCD_WIDTH; x++) {
            uint16_t px = src[x];
            uint8_t hi = px >> 8;
            uint8_t lo = px & 0xFF;
            // Pixel doubled horizontally
            *dst++ = hi; *dst++ = lo;
            *dst++ = hi; *dst++ = lo;
        }

        if (use_pio) {
            // Send via DMA to PIO
            st7789_wait_idle();
            gpio_put(DISPLAY_DC, 1);

            dma_channel_config c = dma_channel_get_default_config(dma_chan);
            channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
            channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true));
            channel_config_set_read_increment(&c, true);
            channel_config_set_write_increment(&c, false);

            dma_channel_configure(dma_chan, &c,
                &pio->txf[sm],
                line_buf,
                DISPLAY_WIDTH * 2,
                true
            );
            dma_channel_wait_for_finish_blocking(dma_chan);
        } else {
            if (y == 0) gpio_put(DISPLAY_DC, 1);
            for (int i = 0; i < DISPLAY_WIDTH * 2; i++) {
                bb_write_byte(line_buf[i]);
            }
        }
    }
}
