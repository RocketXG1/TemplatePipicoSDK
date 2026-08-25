#ifndef TEST_HARDWARE_PWM_H
#define TEST_HARDWARE_PWM_H

#include "pico/stdlib.h"

constexpr uint PWM_CHAN_A = 0;
constexpr uint PWM_CHAN_B = 1;

uint pwm_gpio_to_slice_num(uint gpio);
uint pwm_gpio_to_channel(uint gpio);
void pwm_set_wrap(uint slice, uint16_t wrap);
void pwm_set_clkdiv(uint slice, float divider);
void pwm_set_chan_level(uint slice, uint channel, uint16_t level);
void pwm_set_enabled(uint slice, bool enabled);

#endif
