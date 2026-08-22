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

int PwmManager::registerPwmOutput(const char* name, uint gpioPin, uint32_t frequencyHz, uint32_t periodSteps, bool requireExclusiveSlice) {
    auto fail = [this, name](int code, const char* message) {
        std::printf("ERROR PWM: %s\n", message);
        saveRegistrationResult(name, code);
        return code;
    };
    if (isFull()) return fail(PWM_ERROR_REGISTRY_FULL, "registry is full.");
    if (name == nullptr || name[0] == '\0') return fail(PWM_ERROR_INVALID_NAME, "invalid name.");
    if (nameAlreadyUsed(name)) return fail(PWM_ERROR_NAME_USED, "name already used.");
    if (gpioPin >= 30) return fail(PWM_ERROR_GPIO_INVALID, "invalid GPIO.");
    if (gpioAlreadyUsed(gpioPin)) return fail(PWM_ERROR_GPIO_USED, "GPIO already used.");
    if (frequencyHz == 0 || periodSteps == 0 || periodSteps > 65536) {
        return fail(PWM_ERROR_INVALID_PERIOD_STEPS, "frequency must be non-zero and periodSteps must be between 1 and 65536.");
    }

    const uint newSlice = pwm_gpio_to_slice_num(gpioPin);
    const uint newChannel = pwm_gpio_to_channel(gpioPin);
    const uint16_t calculatedWrap = static_cast<uint16_t>(periodSteps - 1);
    for (uint i = 0; i < outputCount; ++i) {
        const PwmOutputInfo& current = outputs[i];
        if (!current.used || current.sliceNum != newSlice) continue;
        if (requireExclusiveSlice || current.requireExclusiveSlice) {
            return fail(PWM_ERROR_SLICE_CONFLICT, "requested PWM slice is exclusive or already exclusively assigned.");
        }
        if (current.channel == newChannel) {
            return fail(PWM_ERROR_CHANNEL_CONFLICT, "requested PWM slice channel is already assigned.");
        }
        if (current.frequencyHz != frequencyHz || current.periodSteps != periodSteps) {
            return fail(PWM_ERROR_SLICE_CONFLICT, "outputs sharing a slice must use the same frequency and periodSteps.");
        }
    }

    PwmOutputInfo& output = outputs[outputCount++];
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
    saveRegistrationResult(name, PWM_OK);
    std::printf("PWM registered: %s | GPIO %u | slice %u | channel %s | freq %lu Hz | periodSteps %lu | wrap %u | exclusive=%s\n",
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
