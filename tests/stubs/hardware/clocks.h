#ifndef TEST_HARDWARE_CLOCKS_H
#define TEST_HARDWARE_CLOCKS_H

#include <cstdint>

enum clock_index { clk_sys };
uint32_t clock_get_hz(clock_index clock);

#endif
