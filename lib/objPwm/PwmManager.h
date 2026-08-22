#ifndef PWM_MANAGER_H
#define PWM_MANAGER_H

#include <cstdint>

#include "pico/stdlib.h"

class PwmManager {
public:
    static constexpr uint MAX_PWM_OUTPUTS = 8;
    static constexpr uint PWM_NAME_MAX_LENGTH = 16;
    static constexpr uint MAX_PWM_REGISTRATION_ATTEMPTS = 16;

    static constexpr int PWM_OK = 0;
    static constexpr int PWM_ERROR_INVALID_NAME = -1;
    static constexpr int PWM_ERROR_NOT_FOUND = -2;
    static constexpr int PWM_ERROR_NOT_INITIALIZED = -3;
    static constexpr int PWM_ERROR_INVALID_PERIOD_STEPS = -4;
    static constexpr int PWM_ERROR_REGISTRY_FULL = -5;
    static constexpr int PWM_ERROR_GPIO_INVALID = -6;
    static constexpr int PWM_ERROR_GPIO_USED = -7;
    static constexpr int PWM_ERROR_NAME_USED = -8;
    static constexpr int PWM_ERROR_SLICE_CONFLICT = -9;
    static constexpr int PWM_ERROR_CHANNEL_CONFLICT = -10;

    struct PwmOutputConfig {
        char name[PWM_NAME_MAX_LENGTH];
        uint gpioPin;
        uint sliceNum;
        uint channel;
        uint32_t frequencyHz;
        uint32_t periodSteps;
        uint16_t wrap;
        bool requireExclusiveSlice;
        bool used;
    };

    PwmManager();

    bool isFull() const;
    bool nameAlreadyUsed(const char* name) const;
    bool gpioAlreadyUsed(uint gpioPin) const;
    int registerPwmOutput(
        const char* name,
        uint gpioPin,
        uint32_t frequencyHz,
        uint32_t periodSteps,
        bool requireExclusiveSlice
    );
    bool validateRegistrationStatus() const;
    int getPwmConfigByName(const char* selectedName, PwmOutputConfig& outputConfig) const;
    void printPwmMap() const;

private:
    struct PwmOutputInfo : PwmOutputConfig {};

    struct PwmRegistrationResult {
        char name[PWM_NAME_MAX_LENGTH];
        int resultCode;
        bool used;
    };

    PwmOutputInfo outputs[MAX_PWM_OUTPUTS];
    uint outputCount;
    PwmRegistrationResult registrationResults[MAX_PWM_REGISTRATION_ATTEMPTS];
    uint registrationResultCount;

    bool textEquals(const char* a, const char* b) const;
    void saveRegistrationResult(const char* name, int resultCode);
};

#endif
