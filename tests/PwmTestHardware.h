#ifndef PWM_TEST_HARDWARE_H
#define PWM_TEST_HARDWARE_H

#include <cstdint>

#include "pico/stdlib.h"

namespace TestHardware {
struct GpioState {
    int function;
    bool output;
};

struct SliceState {
    uint16_t wrap;
    float divider;
    uint16_t level[2];
    bool enabled;
};

void reset();
const GpioState& gpio(uint pin);
const SliceState& slice(uint number);
}

#endif
