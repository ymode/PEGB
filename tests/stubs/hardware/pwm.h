unsigned pwm_gpio_to_slice_num(unsigned pin);
void pwm_set_gpio_level(unsigned pin, unsigned level);
void pwm_set_wrap(unsigned slice, unsigned wrap);
void pwm_set_clkdiv(unsigned slice, float div);
void pwm_set_enabled(unsigned slice, bool enabled);
