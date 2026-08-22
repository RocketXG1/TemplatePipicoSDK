#include "PwmOutput.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "hardware/clocks.h"
#include "hardware/pwm.h"

PwmOutput::PwmOutput(const PwmManager& selectedPwmManager)
    : pwmManager(&selectedPwmManager), name{}, gpioPin(0), sliceNum(0), channel(0), frequencyHz(0),
      periodSteps(0), wrap(0), clkDiv(0.0f), initialized(false) {}

int PwmOutput::init(const char* selectedName) {
    if (pwmManager == nullptr) return PwmManager::PWM_ERROR_NOT_INITIALIZED;
    PwmManager::PwmOutputConfig config{};
    const int result = pwmManager->getPwmConfigByName(selectedName, config);
    if (result != PwmManager::PWM_OK) return result;
    if (config.frequencyHz == 0 || config.periodSteps == 0 || config.periodSteps > 65536) return PwmManager::PWM_ERROR_INVALID_PERIOD_STEPS;

    std::strncpy(name, config.name, PwmManager::PWM_NAME_MAX_LENGTH - 1);
    name[PwmManager::PWM_NAME_MAX_LENGTH - 1] = '\0';
    gpioPin = config.gpioPin;
    sliceNum = config.sliceNum;
    channel = config.channel;
    frequencyHz = config.frequencyHz;
    periodSteps = config.periodSteps;
    wrap = config.wrap;
    gpio_set_function(gpioPin, GPIO_FUNC_PWM);
    clkDiv = static_cast<float>(clock_get_hz(clk_sys)) / (static_cast<float>(frequencyHz) * static_cast<float>(periodSteps));
    pwm_set_wrap(sliceNum, wrap);
    pwm_set_clkdiv(sliceNum, clkDiv);
    pwm_set_chan_level(sliceNum, channel, 0);
    pwm_set_enabled(sliceNum, true);
    initialized = true;
    return PwmManager::PWM_OK;
}

bool PwmOutput::isInitialized() const { return initialized; }
const char* PwmOutput::getName() const { return name; }
uint PwmOutput::getGpioPin() const { return gpioPin; }
uint PwmOutput::getSliceNum() const { return sliceNum; }
uint PwmOutput::getChannel() const { return channel; }
uint32_t PwmOutput::getFrequencyHz() const { return frequencyHz; }
uint32_t PwmOutput::getPeriodSteps() const { return periodSteps; }
uint16_t PwmOutput::getWrap() const { return wrap; }

void PwmOutput::setLevel(uint32_t level) {
    if (!initialized) return;
    const uint32_t maximumLevel = std::min<uint32_t>(periodSteps, 65535);
    pwm_set_chan_level(sliceNum, channel, static_cast<uint16_t>(std::min(level, maximumLevel)));
}

void PwmOutput::setDutyPercent(float dutyPercent) {
    if (!initialized) return;
    dutyPercent = std::max(0.0f, std::min(dutyPercent, 100.0f));
    setLevel(static_cast<uint32_t>(static_cast<float>(periodSteps) * dutyPercent / 100.0f));
}

void PwmOutput::setServoAngle180(float angleDegrees) {
    if (!initialized) return;
    angleDegrees = std::max(0.0f, std::min(angleDegrees, 180.0f));
    constexpr float minPulseUs = 500.0f;
    constexpr float maxPulseUs = 2500.0f;
    const float pulseWidthUs = minPulseUs + angleDegrees / 180.0f * (maxPulseUs - minPulseUs);
    const float periodUs = 1000000.0f / static_cast<float>(frequencyHz);
    setLevel(static_cast<uint32_t>(pulseWidthUs / periodUs * static_cast<float>(periodSteps)));
}

void rampPwmUpDown(PwmOutput& pwm, const char* pwmName, float startPercent, float endPercent, float stepPercent, uint delayMs) {
    if (stepPercent <= 0.0f || startPercent > endPercent) return;
    std::printf("Starting ascending ramp: %s\n", pwmName);
    for (float duty = startPercent; duty <= endPercent; duty += stepPercent) { pwm.setDutyPercent(duty); sleep_ms(delayMs); }
    std::printf("Starting descending ramp: %s\n", pwmName);
    for (float duty = endPercent; duty >= startPercent; duty -= stepPercent) { pwm.setDutyPercent(duty); sleep_ms(delayMs); }
}

void sweepServo180(PwmOutput& servo, const char* servoName, uint delayMs) {
    std::printf("Starting servo sweep: %s\n", servoName);
    for (float angle = 0.0f; angle <= 180.0f; angle += 5.0f) { servo.setServoAngle180(angle); sleep_ms(delayMs); }
    for (float angle = 180.0f; angle >= 0.0f; angle -= 5.0f) { servo.setServoAngle180(angle); sleep_ms(delayMs); }
}
