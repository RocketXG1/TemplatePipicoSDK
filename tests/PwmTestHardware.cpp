#include "PwmTestHardware.h"

#include <array>

#include "hardware/clocks.h"
#include "hardware/pwm.h"

namespace {
std::array<TestHardware::GpioState, 30> gpios{};
std::array<TestHardware::SliceState, 8> slices{};
}

namespace TestHardware {
void reset() {
    gpios = {};
    slices = {};
}

const GpioState& gpio(uint pin) { return gpios.at(pin); }
const SliceState& slice(uint number) { return slices.at(number); }
}

void gpio_set_function(uint gpio, int function) { gpios.at(gpio).function = function; }
void gpio_set_dir(uint gpio, bool output) { gpios.at(gpio).output = output; }
void sleep_ms(uint) {}

uint pwm_gpio_to_slice_num(uint gpio) { return (gpio >> 1U) & 7U; }
uint pwm_gpio_to_channel(uint gpio) { return gpio & 1U; }
void pwm_set_wrap(uint slice, uint16_t wrap) { slices.at(slice).wrap = wrap; }
void pwm_set_clkdiv(uint slice, float divider) { slices.at(slice).divider = divider; }
void pwm_set_chan_level(uint slice, uint channel, uint16_t level) { slices.at(slice).level[channel] = level; }
void pwm_set_enabled(uint slice, bool enabled) { slices.at(slice).enabled = enabled; }

uint32_t clock_get_hz(clock_index) { return 125000000U; }
