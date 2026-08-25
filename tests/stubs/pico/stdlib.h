#ifndef TEST_PICO_STDLIB_H
#define TEST_PICO_STDLIB_H

#include <cstdint>

using uint = unsigned int;

constexpr int GPIO_FUNC_SIO = 5;
constexpr int GPIO_FUNC_PWM = 4;
constexpr bool GPIO_IN = false;

void gpio_set_function(uint gpio, int function);
void gpio_set_dir(uint gpio, bool output);
void stdio_init_all();
void tight_loop_contents();
void sleep_ms(uint milliseconds);

#endif
