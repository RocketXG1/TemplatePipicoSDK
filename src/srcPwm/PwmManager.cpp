#include "PwmManager.h"

#include <cstdio>
#include <cstring>

#include "hardware/pwm.h"

PwmManager::PwmManager() : outputs{}, outputCount(0), registrationResults{}, registrationResultCount(0) {}

bool PwmManager::textEquals(const char* a, const char* b) const {
    return a != nullptr && b != nullptr && std::strcmp(a, b) == 0;
}

void PwmManager::saveRegistrationResult(const char* name, int resultCode) {
    if (registrationResultCount >= MAX_PWM_REGISTRATION_ATTEMPTS) return;
    PwmRegistrationResult& result = registrationResults[registrationResultCount++];
    std::strncpy(result.name, name == nullptr || name[0] == '\0' ? "UNKNOWN" : name, PWM_NAME_MAX_LENGTH - 1);
    result.name[PWM_NAME_MAX_LENGTH - 1] = '\0';
    result.resultCode = resultCode;
    result.used = true;
}

bool PwmManager::isFull() const { return outputCount >= MAX_PWM_OUTPUTS; }

bool PwmManager::nameAlreadyUsed(const char* name) const {
    for (uint i = 0; i < outputCount; ++i) if (outputs[i].used && textEquals(outputs[i].name, name)) return true;
    return false;
}

bool PwmManager::gpioAlreadyUsed(uint gpioPin) const {
    for (uint i = 0; i < outputCount; ++i) if (outputs[i].used && outputs[i].gpioPin == gpioPin) return true;
    return false;
}

int PwmManager::findPwmOutputIndex(const char* name) const {
    if (name == nullptr || name[0] == '\0') return -1;
    for (uint i = 0; i < outputCount; ++i) {
        if (outputs[i].used && textEquals(outputs[i].name, name)) return static_cast<int>(i);
    }
    return -1;
}

int PwmManager::validatePwmOutput(
    const char* name,
    uint gpioPin,
    uint32_t frequencyHz,
    uint32_t periodSteps,
    bool requireExclusiveSlice,
    int ignoredOutputIndex
) const {
    if (name == nullptr || name[0] == '\0') return PWM_ERROR_INVALID_NAME;
    if (gpioPin >= 30) return PWM_ERROR_GPIO_INVALID;
    if (frequencyHz == 0 || periodSteps == 0 || periodSteps > 65536) return PWM_ERROR_INVALID_PERIOD_STEPS;

    const uint newSlice = pwm_gpio_to_slice_num(gpioPin);
    const uint newChannel = pwm_gpio_to_channel(gpioPin);
    for (uint i = 0; i < outputCount; ++i) {
        if (static_cast<int>(i) == ignoredOutputIndex) continue;
        const PwmOutputInfo& current = outputs[i];
        if (!current.used) continue;
        if (textEquals(current.name, name)) return PWM_ERROR_NAME_USED;
        if (current.gpioPin == gpioPin) return PWM_ERROR_GPIO_USED;
        if (current.sliceNum != newSlice) continue;
        if (requireExclusiveSlice || current.requireExclusiveSlice) {
            return PWM_ERROR_SLICE_CONFLICT;
        }
        if (current.channel == newChannel) return PWM_ERROR_CHANNEL_CONFLICT;
        if (current.frequencyHz != frequencyHz || current.periodSteps != periodSteps) {
            return PWM_ERROR_SLICE_CONFLICT;
        }
    }
    return PWM_OK;
}

void PwmManager::assignPwmOutput(
    PwmOutputInfo& output,
    const char* name,
    uint gpioPin,
    uint32_t frequencyHz,
    uint32_t periodSteps,
    bool requireExclusiveSlice
) {
    const uint newSlice = pwm_gpio_to_slice_num(gpioPin);
    const uint newChannel = pwm_gpio_to_channel(gpioPin);
    const uint16_t calculatedWrap = static_cast<uint16_t>(periodSteps - 1);

    std::strncpy(output.name, name, PWM_NAME_MAX_LENGTH - 1);
    output.name[PWM_NAME_MAX_LENGTH - 1] = '\0';
    output.gpioPin = gpioPin;
    output.sliceNum = newSlice;
    output.channel = newChannel;
    output.frequencyHz = frequencyHz;
    output.periodSteps = periodSteps;
    output.wrap = calculatedWrap;
    output.requireExclusiveSlice = requireExclusiveSlice;
    output.used = true;
}

int PwmManager::registerPwmOutput(const char* name, uint gpioPin, uint32_t frequencyHz, uint32_t periodSteps, bool requireExclusiveSlice) {
    int result = PWM_OK;
    if (isFull()) {
        result = PWM_ERROR_REGISTRY_FULL;
    } else {
        result = validatePwmOutput(name, gpioPin, frequencyHz, periodSteps, requireExclusiveSlice, -1);
    }
    if (result != PWM_OK) {
        std::printf("ERROR PWM: registration failed with code %d.\n", result);
        saveRegistrationResult(name, result);
        return result;
    }

    PwmOutputInfo& output = outputs[outputCount++];
    assignPwmOutput(output, name, gpioPin, frequencyHz, periodSteps, requireExclusiveSlice);
    saveRegistrationResult(name, PWM_OK);
    std::printf("PWM registered: %s | GPIO %u | slice %u | channel %s | freq %lu Hz | periodSteps %lu | wrap %u | exclusive=%s\n",
        output.name, output.gpioPin, output.sliceNum, output.channel == PWM_CHAN_A ? "A" : "B",
        static_cast<unsigned long>(output.frequencyHz), static_cast<unsigned long>(output.periodSteps), output.wrap,
        output.requireExclusiveSlice ? "YES" : "NO");
    return PWM_OK;
}

int PwmManager::updatePwmOutput(const char* name, uint gpioPin, uint32_t frequencyHz, uint32_t periodSteps, bool requireExclusiveSlice) {
    const int outputIndex = findPwmOutputIndex(name);
    if (outputIndex < 0) {
        std::printf("ERROR PWM: output name not found: %s\n", name == nullptr ? "UNKNOWN" : name);
        return name == nullptr || name[0] == '\0' ? PWM_ERROR_INVALID_NAME : PWM_ERROR_NOT_FOUND;
    }

    const int result = validatePwmOutput(
        name,
        gpioPin,
        frequencyHz,
        periodSteps,
        requireExclusiveSlice,
        outputIndex
    );
    if (result != PWM_OK) {
        std::printf("ERROR PWM: update for %s failed with code %d.\n", name, result);
        return result;
    }

    PwmOutputInfo& output = outputs[outputIndex];
    assignPwmOutput(output, name, gpioPin, frequencyHz, periodSteps, requireExclusiveSlice);
    std::printf("PWM updated: %s | GPIO %u | slice %u | channel %s | freq %lu Hz | periodSteps %lu | wrap %u | exclusive=%s\n",
        output.name, output.gpioPin, output.sliceNum, output.channel == PWM_CHAN_A ? "A" : "B",
        static_cast<unsigned long>(output.frequencyHz), static_cast<unsigned long>(output.periodSteps), output.wrap,
        output.requireExclusiveSlice ? "YES" : "NO");
    return PWM_OK;
}

bool PwmManager::validateRegistrationStatus() const {
    bool allOk = true;
    std::printf("\n===== PWM REGISTRATION STATUS =====\nPWM successfully created: %u\nPWM registration attempts: %u\n", outputCount, registrationResultCount);
    for (uint i = 0; i < registrationResultCount; ++i) {
        if (registrationResults[i].used && registrationResults[i].resultCode != PWM_OK) {
            allOk = false;
            std::printf("ERROR: PWM %s failed with code %d\n", registrationResults[i].name, registrationResults[i].resultCode);
        }
    }
    if (allOk) std::printf("All PWM outputs were registered correctly.\n");
    std::printf("===================================\n\n");
    return allOk;
}

int PwmManager::getPwmConfigByName(const char* selectedName, PwmOutputConfig& outputConfig) const {
    if (selectedName == nullptr || selectedName[0] == '\0') return PWM_ERROR_INVALID_NAME;
    for (uint i = 0; i < outputCount; ++i) {
        if (outputs[i].used && textEquals(outputs[i].name, selectedName)) {
            static_cast<PwmOutputConfig&>(outputConfig) = outputs[i];
            return PWM_OK;
        }
    }
    std::printf("ERROR PWM: output name not found: %s\n", selectedName);
    return PWM_ERROR_NOT_FOUND;
}

void PwmManager::printPwmMap() const {
    std::printf("\n===== PWM MAP =====\n");
    for (uint i = 0; i < outputCount; ++i) {
        const PwmOutputInfo& output = outputs[i];
        if (!output.used) continue;
        std::printf("[%u] %s | GPIO %u | slice %u | channel %s | freq %lu Hz | periodSteps %lu | wrap %u | exclusive=%s\n",
            i, output.name, output.gpioPin, output.sliceNum, output.channel == PWM_CHAN_A ? "A" : "B",
            static_cast<unsigned long>(output.frequencyHz), static_cast<unsigned long>(output.periodSteps), output.wrap,
            output.requireExclusiveSlice ? "YES" : "NO");
    }
    std::printf("===================\n\n");
}
