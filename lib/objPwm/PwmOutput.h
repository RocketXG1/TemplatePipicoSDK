#ifndef PWM_OUTPUT_H
#define PWM_OUTPUT_H

#include "PwmManager.h"

class PwmOutput {
public:
    explicit PwmOutput(PwmManager& selectedPwmManager);
    ~PwmOutput();

    PwmOutput(const PwmOutput&) = delete;
    PwmOutput& operator=(const PwmOutput&) = delete;

    int init(const char* selectedName);
    bool isInitialized() const;
    const char* getName() const;
    uint getGpioPin() const;
    uint getSliceNum() const;
    uint getChannel() const;
    uint32_t getFrequencyHz() const;
    uint32_t getPeriodSteps() const;
    uint16_t getWrap() const;
    void setLevel(uint32_t level);
    void setDutyPercent(float dutyPercent);
    void setServoAngle180(float angleDegrees);

private:
    friend class PwmManager;

    PwmManager* pwmManager;
    char name[PwmManager::PWM_NAME_MAX_LENGTH];
    uint gpioPin;
    uint sliceNum;
    uint channel;
    uint32_t frequencyHz;
    uint32_t periodSteps;
    uint16_t wrap;
    float clkDiv;
    bool initialized;
    PwmOutput* nextAttached;

    void applyConfig(const PwmManager::PwmOutputConfig& config);
    void refreshIfNamed(const char* updatedName);
};

void rampPwmUpDown(
    PwmOutput& pwm,
    const char* pwmName,
    float startPercent,
    float endPercent,
    float stepPercent,
    uint delayMs
);

void sweepServo180(PwmOutput& servo, const char* servoName, uint delayMs);

#endif
