#include "audio.h"
#include "audio_queue.h"
#include "timing.h"
#include "hw_config.h"

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"

#define AUDIO_SAMPLE_RATE PEGB_SAMPLE_RATE
#include "minigb_apu.h"

static struct minigb_apu_ctx apu;
static audio_queue_t queue;
static struct repeating_timer audio_timer;
static uint32_t timer_remainder;
static uint32_t frame_remainder;
static bool playing;

uint8_t audio_read(uint16_t addr) {
    return minigb_apu_audio_read(&apu, addr);
}

void audio_write(uint16_t addr, uint8_t val) {
    minigb_apu_audio_write(&apu, addr, val);
}

static bool audio_timer_callback(struct repeating_timer *timer) {
    // Start with roughly two frames buffered; recover the same way after underrun.
    if (!playing && queue.count >= 1024)
        playing = true;
    uint8_t sample = playing ? audio_queue_pop(&queue) : AUDIO_SILENCE;
    if (!queue.count)
        playing = false;
    pwm_set_gpio_level(PIN_AUDIO, sample);
    // Pico SDK reads delay_us after the callback; negatives preserve deadlines.
    timer->delay_us = -(int64_t)audio_clock_next_us(&timer_remainder);
    return true;
}

void audio_pause(void) {
    uint32_t irq_state = save_and_disable_interrupts();
    audio_queue_reset(&queue);
    playing = false;
    pwm_set_gpio_level(PIN_AUDIO, AUDIO_SILENCE);
    restore_interrupts(irq_state);
}

void audio_reset(void) {
    audio_pause();
    minigb_apu_audio_init(&apu);
    frame_remainder = 0;
}

void audio_init(void) {
    gpio_init(PIN_AMP_EN);
    gpio_set_dir(PIN_AMP_EN, GPIO_OUT);
    gpio_put(PIN_AMP_EN, 1);
    gpio_set_function(PIN_AUDIO, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PIN_AUDIO);
    pwm_set_wrap(slice, 255);
    pwm_set_clkdiv(slice, 1.0f);
    pwm_set_gpio_level(PIN_AUDIO, AUDIO_SILENCE);
    pwm_set_enabled(slice, true);
    audio_reset();
    int64_t delay = -(int64_t)audio_clock_next_us(&timer_remainder);
    if (!add_repeating_timer_us(delay, audio_timer_callback, NULL, &audio_timer))
        panic("Could not start audio timer");
}

void audio_play_frame(void) {
    // The upstream AUDIO_SAMPLES macro uses floating point, not a C11 array bound.
    static audio_sample_t raw[PEGB_FRAME_SAMPLES_MAX * 2];
    uint8_t mono[PEGB_FRAME_SAMPLES_MAX];
    minigb_apu_audio_callback(&apu, raw);
    unsigned count = audio_frame_samples(&frame_remainder);
    // minigb_apu generates a fixed 548 samples. Stretch to 548/549 to keep
    // the sample count aligned with complete DMG frames (548.625 on average).
    for (unsigned i = 0; i < count; ++i) {
        unsigned src = i * AUDIO_SAMPLES / count;
        int32_t mixed = ((int32_t)raw[src * 2] + raw[src * 2 + 1]) / 2;
        int pwm = (mixed + 32768) >> 8;
        mono[i] = (uint8_t)(128 + (pwm - 128) / 2);
    }

    // The ISR cannot observe a partial enqueue. Only the short copies/metadata
    // update run with interrupts disabled; synthesis and waiting run outside.
    while (true) {
        uint32_t irq_state = save_and_disable_interrupts();
        bool queued = audio_queue_push(&queue, mono, count);
        restore_interrupts(irq_state);
        if (queued)
            return;
        sleep_us(100); // Backpressure: let the ISR consume queued samples.
    }
}
