#include "PwmManager.h"

#include <cstdio>
#include <cstring>

#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "PwmOutput.h"

PwmManager::PwmManager()
    : outputs{}, outputCount(0), validationResults{}, validationResultCount(0), attachedOutputHead(nullptr) {}

bool PwmManager::textEquals(const char* a, const char* b) const {
    return a != nullptr && b != nullptr && std::strcmp(a, b) == 0;
}

void PwmManager::saveRegistrationResult(const char* name, int resultCode) {
    const char* savedName = name == nullptr || name[0] == '\0' ? "UNKNOWN" : name;
    for (uint i = 0; i < validationResultCount; ++i) {
        if (validationResults[i].used && textEquals(validationResults[i].name, savedName)) {
            validationResults[i].resultCode = resultCode;
            return;
        }
    }
    if (validationResultCount >= MAX_PWM_VALIDATION_RESULTS) return;
    PwmValidationResult& result = validationResults[validationResultCount++];
    std::strncpy(result.name, savedName, PWM_NAME_MAX_LENGTH - 1);
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
    if (std::strlen(name) >= PWM_NAME_MAX_LENGTH) return PWM_ERROR_INVALID_NAME;
    if (gpioPin >= 30) return PWM_ERROR_GPIO_INVALID;
    if (frequencyHz == 0 || periodSteps == 0 || periodSteps > 65536) return PWM_ERROR_INVALID_PERIOD_STEPS;

    constexpr double minimumDivider = 1.0;
    constexpr double maximumDivider = 255.0 + 15.0 / 16.0;
    const double divider = static_cast<double>(clock_get_hz(clk_sys)) /
        (static_cast<double>(frequencyHz) * static_cast<double>(periodSteps));
    if (divider < minimumDivider || divider > maximumDivider) return PWM_ERROR_INVALID_CLOCK_DIVIDER;

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

int PwmManager::registerPwmOutput(
    const char* name,
    uint gpioPin,
    uint32_t frequencyHz,
    uint32_t periodSteps,
    bool requireExclusiveSlice,
    PwmRegistrationAction* action
) {
    if (action != nullptr) *action = PwmRegistrationAction::None;
    const int existingOutputIndex = findPwmOutputIndex(name);
    int result = PWM_OK;
    if (existingOutputIndex < 0 && isFull()) {
        result = PWM_ERROR_REGISTRY_FULL;
    } else {
        result = validatePwmOutput(
            name,
            gpioPin,
            frequencyHz,
            periodSteps,
            requireExclusiveSlice,
            existingOutputIndex
        );
    }
    if (result != PWM_OK) {
        std::printf("ERROR PWM: registration failed with code %d.\n", result);
        saveRegistrationResult(name, result);
        return result;
    }

    const bool isUpdate = existingOutputIndex >= 0;
    PwmOutputInfo& output = isUpdate ? outputs[existingOutputIndex] : outputs[outputCount++];
    assignPwmOutput(output, name, gpioPin, frequencyHz, periodSteps, requireExclusiveSlice);
    if (isUpdate) {
        for (PwmOutput* attachedOutput = attachedOutputHead;
             attachedOutput != nullptr;
             attachedOutput = attachedOutput->nextAttached) {
            attachedOutput->refreshIfNamed(name);
        }
    }
    if (action != nullptr) {
        *action = isUpdate ? PwmRegistrationAction::Updated : PwmRegistrationAction::Registered;
    }
    saveRegistrationResult(name, PWM_OK);
    std::printf("PWM %s: %s | GPIO %u | slice %u | channel %s | freq %lu Hz | periodSteps %lu | wrap %u | exclusive=%s\n",
        isUpdate ? "updated" : "registered",
        output.name, output.gpioPin, output.sliceNum, output.channel == PWM_CHAN_A ? "A" : "B",
        static_cast<unsigned long>(output.frequencyHz), static_cast<unsigned long>(output.periodSteps), output.wrap,
        output.requireExclusiveSlice ? "YES" : "NO");
    return PWM_OK;
}

int PwmManager::updatePwmOutput(
    const char* name,
    uint gpioPin,
    uint32_t frequencyHz,
    uint32_t periodSteps,
    bool requireExclusiveSlice
) {
    if (findPwmOutputIndex(name) < 0) {
        saveRegistrationResult(name, PWM_ERROR_NOT_FOUND);
        return PWM_ERROR_NOT_FOUND;
    }
    PwmRegistrationAction action = PwmRegistrationAction::None;
    return registerPwmOutput(name, gpioPin, frequencyHz, periodSteps, requireExclusiveSlice, &action);
}

void PwmManager::attachOutput(PwmOutput* output) {
    if (output == nullptr) return;
    output->nextAttached = attachedOutputHead;
    attachedOutputHead = output;
}

void PwmManager::detachOutput(PwmOutput* output) {
    PwmOutput** current = &attachedOutputHead;
    while (*current != nullptr) {
        if (*current == output) {
            *current = output->nextAttached;
            output->nextAttached = nullptr;
            return;
        }
        current = &((*current)->nextAttached);
    }
}

bool PwmManager::sliceIsRegistered(uint selectedSliceNum) const {
    for (uint i = 0; i < outputCount; ++i) {
        if (outputs[i].used && outputs[i].sliceNum == selectedSliceNum) return true;
    }
    return false;
}

bool PwmManager::validateRegistrationStatus() const {
    bool allOk = true;
    std::printf("\n===== PWM REGISTRATION STATUS =====\nPWM successfully created: %u\nPWM names evaluated: %u\n", outputCount, validationResultCount);
    for (uint i = 0; i < validationResultCount; ++i) {
        if (validationResults[i].used && validationResults[i].resultCode != PWM_OK) {
            allOk = false;
            std::printf("ERROR: PWM %s failed with code %d\n", validationResults[i].name, validationResults[i].resultCode);
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
