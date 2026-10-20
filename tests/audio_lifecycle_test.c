#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include "../src/audio.c"
#define main firmware_main
#include "../src/main.c"
#undef main

static struct repeating_timer *registered_timer;
static bool irq_off, preempt;
static unsigned timers, waits, levels[4096], level_count;
static uint64_t fake_now;
static unsigned menus, frames, deadlines;
static uint8_t frame_buttons[3];
static jmp_buf stopped;

static void tick(void) {
    assert(!irq_off);
    assert(registered_timer->callback(registered_timer));
}
uint32_t save_and_disable_interrupts(void) {
    bool prior = irq_off;
    if (preempt && !prior && registered_timer) tick();
    irq_off = true;
    return prior ? 0 : 1;
}
void restore_interrupts(uint32_t state) {
    irq_off = !state;
    if (preempt && state && registered_timer) tick();
}
bool add_repeating_timer_us(int64_t delay, bool (*cb)(struct repeating_timer *),
                           void *data, struct repeating_timer *timer) {
    (void)data;
    timer->delay_us = delay; timer->callback = cb; registered_timer = timer;
    ++timers; return true;
}
void pwm_set_gpio_level(unsigned pin, unsigned value) {
    (void)pin;
    assert(value <= 255);
    assert(level_count < sizeof(levels)/sizeof(levels[0]));
    levels[level_count++] = value;
}
unsigned pwm_gpio_to_slice_num(unsigned pin) {return pin;}
void pwm_set_wrap(unsigned slice, unsigned wrap) {(void)slice;(void)wrap;}
void pwm_set_clkdiv(unsigned slice, float div) {(void)slice;(void)div;}
void pwm_set_enabled(unsigned slice, bool enabled) {(void)slice;(void)enabled;}
void gpio_init(unsigned pin) {(void)pin;}
void gpio_set_dir(unsigned pin, bool out) {(void)pin;(void)out;}
void gpio_put(unsigned pin, bool value) {(void)pin;(void)value;}
void gpio_set_function(unsigned pin, unsigned function) {(void)pin;(void)function;}
void sleep_us(uint64_t us) {
    assert(!irq_off); ++waits; fake_now += us;
    for (unsigned i=0; i<4; ++i) tick();
}
void sleep_ms(uint32_t ms) {fake_now += (uint64_t)ms * 1000;}
uint64_t time_us_64(void) {return fake_now;}
void sleep_until(absolute_time_t target) {
    assert(target > fake_now);
    assert(target - fake_now <= 16743);
    fake_now = target; ++deadlines;
}
void panic(const char *msg) {fprintf(stderr,"panic: %s\n",msg); abort();}
void tight_loop_contents(void) {abort();}
void stdio_init_all(void) {}
void vreg_set_voltage(unsigned v) {(void)v;}
void set_sys_clock_khz(unsigned clock, bool required) {(void)clock;(void)required;}
void display_init(void) {}
void display_fill(uint16_t color) {(void)color; abort();}
void display_frame(const uint16_t *fb) {
    (void)fb;
    assert(frames < 3);
    frame_buttons[frames++] = gb.direct.joypad;
    fake_now += 1000; // simulated frame work
    if (frames == 3) longjmp(stopped, 1);
}
void input_init(void) {}
joypad_t input_read(void) {
    joypad_t jp = {.byte = frames == 0 ? 0xFB : (menus == 1 ? 0xF3 : 0xFF)};
    return jp;
}
int menu_select_game(uint16_t *fb, int count, int current) {
    (void)fb;(void)count;(void)current;
    ++menus;
    if (menus == 1) return 0;
    fake_now += 5000000; // user spends five seconds in the menu
    return 1;
}

int main(void) {
    audio_init(); emu_init();
    assert(timers == 1);
    uint8_t volume = audio_read(0xFF24), wave = audio_read(0xFF30);
    audio_write(0xFF24, 0x12); audio_write(0xFF30, 0x55);
    uint8_t old[1024];
    for (unsigned i=0; i<sizeof old; ++i) old[i]=(uint8_t)i;
    assert(audio_queue_push(&queue, old, sizeof old));
    current_rom = 1; emu_init();
    assert(audio_read(0xFF24) == volume && audio_read(0xFF30) == wave);
    assert(queue.count == 0 && levels[level_count-1] == AUDIO_SILENCE);
    assert(timers == 1); // switching games must not start another timer

    assert(audio_queue_push(&queue, old, sizeof old));
    playing = true; preempt = true; level_count = 0;
    audio_play_frame(); // ISR at both edges of the real publication critical section
    preempt = false;
    assert(level_count == 2 && levels[0] == 0 && levels[1] == 1);
    assert(queue.count == sizeof old - 2 + 548);
    for (unsigned i=2; i<sizeof old; ++i) assert(audio_queue_pop(&queue) == (uint8_t)i);
    assert(queue.count == 548); // first new sample is still unread
    audio_pause();
    assert(queue.count == 0 && !playing);
    tick(); assert(levels[level_count-1] == AUDIO_SILENCE);

    audio_reset(); waits = 0;
    for (unsigned i=0; i<4; ++i) audio_play_frame();
    assert(waits > 0 && queue.count <= AUDIO_QUEUE_CAPACITY);
    while (queue.count) tick();
    tick(); assert(levels[level_count-1] == AUDIO_SILENCE);
    audio_write(0xFF24, 0x12);
    audio_pause(); assert(audio_read(0xFF24) == 0x12); // menu cancel preserves APU
    puts("PASS: real APU reset, ISR publication boundaries, backpressure and neutral underruns");

    level_count = 0; current_rom = 0;
    if (!setjmp(stopped)) firmware_main();
    assert(menus == 2 && frames == 3 && deadlines == 2 && current_rom == 1);
    assert(frame_buttons[0] == 0xFB); // Select reaches the actual emulator
    assert(frame_buttons[1] == 0xFF); // menu chord is consumed
    puts("PASS: actual main loop delivers Select, switches games and rebases pacing after menu");
}
