#ifndef TEST_PICO_STDLIB_H
#define TEST_PICO_STDLIB_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
typedef unsigned uint;
typedef uint64_t absolute_time_t;
#define GPIO_OUT 1
#define GPIO_IN 0
#define GPIO_FUNC_PWM 4
struct repeating_timer { int64_t delay_us; bool (*callback)(struct repeating_timer *); };
void gpio_init(uint pin);
void gpio_set_dir(uint pin, bool out);
void gpio_put(uint pin, bool value);
void gpio_set_function(uint pin, uint function);
void gpio_pull_up(uint pin);
bool gpio_get(uint pin);
void sleep_ms(uint32_t ms);
void sleep_us(uint64_t us);
void sleep_until(absolute_time_t target);
uint64_t time_us_64(void);
static inline absolute_time_t from_us_since_boot(uint64_t us) { return us; }
bool add_repeating_timer_us(int64_t delay, bool (*callback)(struct repeating_timer *),
                            void *data, struct repeating_timer *timer);
void panic(const char *message);
void tight_loop_contents(void);
void stdio_init_all(void);
#endif
