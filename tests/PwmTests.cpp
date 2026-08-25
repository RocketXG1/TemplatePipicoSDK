#include <cstdio>

#include "PwmManager.h"
#include "PwmOutput.h"
#include "PwmTestHardware.h"
#include "hardware/pwm.h"

namespace {
int failures = 0;

#define EXPECT_EQ(expected, actual) do { \
    const auto expectedValue = (expected); \
    const auto actualValue = (actual); \
    if (expectedValue != actualValue) { \
        std::printf("FAIL %s:%d: expected %lld, got %lld\n", __FILE__, __LINE__, \
            static_cast<long long>(expectedValue), static_cast<long long>(actualValue)); \
        ++failures; \
    } \
} while (false)

#define EXPECT_TRUE(value) EXPECT_EQ(true, static_cast<bool>(value))
#define EXPECT_FALSE(value) EXPECT_EQ(false, static_cast<bool>(value))

void testCreateUpdateAndAutomaticRefresh() {
    TestHardware::reset();
    PwmManager manager;
    PwmManager::PwmRegistrationAction action = PwmManager::PwmRegistrationAction::None;
    EXPECT_EQ(PwmManager::PWM_OK, manager.registerPwmOutput("MOTOR", 2, 1000, 1000, true, &action));
    EXPECT_EQ(PwmManager::PwmRegistrationAction::Registered, action);

    PwmOutput output(manager);
    EXPECT_EQ(PwmManager::PWM_OK, output.init("MOTOR"));
    output.setDutyPercent(50.0f);
    EXPECT_EQ(500, TestHardware::slice(1).level[PWM_CHAN_A]);

    EXPECT_EQ(PwmManager::PWM_OK, manager.updatePwmOutput("MOTOR", 4, 2000, 500, true));
    EXPECT_EQ(4, output.getGpioPin());
    EXPECT_EQ(2, output.getSliceNum());
    EXPECT_EQ(2000, output.getFrequencyHz());
    EXPECT_EQ(GPIO_FUNC_SIO, TestHardware::gpio(2).function);
    EXPECT_EQ(0, TestHardware::slice(1).level[PWM_CHAN_A]);
    EXPECT_FALSE(TestHardware::slice(1).enabled);
    EXPECT_EQ(GPIO_FUNC_PWM, TestHardware::gpio(4).function);
    EXPECT_TRUE(TestHardware::slice(2).enabled);
}

void testConflictsAndFailedUpdatePreservesConfiguration() {
    PwmManager manager;
    EXPECT_EQ(PwmManager::PWM_OK, manager.registerPwmOutput("FIRST", 4, 1000, 1000, false));
    EXPECT_EQ(PwmManager::PWM_ERROR_GPIO_USED, manager.registerPwmOutput("GPIO", 4, 1000, 1000, false));
    EXPECT_EQ(PwmManager::PWM_ERROR_CHANNEL_CONFLICT, manager.registerPwmOutput("CHANNEL", 20, 1000, 1000, false));
    EXPECT_EQ(PwmManager::PWM_ERROR_SLICE_CONFLICT, manager.registerPwmOutput("TIMING", 5, 2000, 1000, false));
    EXPECT_EQ(PwmManager::PWM_ERROR_SLICE_CONFLICT, manager.registerPwmOutput("EXCLUSIVE", 5, 1000, 1000, true));
    EXPECT_EQ(PwmManager::PWM_OK, manager.registerPwmOutput("SECOND", 5, 1000, 1000, false));
    EXPECT_EQ(PwmManager::PWM_ERROR_CHANNEL_CONFLICT, manager.updatePwmOutput("FIRST", 21, 1000, 1000, false));

    PwmManager::PwmOutputConfig config{};
    EXPECT_EQ(PwmManager::PWM_OK, manager.getPwmConfigByName("FIRST", config));
    EXPECT_EQ(4, config.gpioPin);
}

void testInputAndClockDividerValidation() {
    PwmManager manager;
    EXPECT_EQ(PwmManager::PWM_ERROR_INVALID_NAME,
        manager.registerPwmOutput("1234567890123456", 2, 1000, 1000, false));
    EXPECT_EQ(PwmManager::PWM_ERROR_GPIO_INVALID, manager.registerPwmOutput("GPIO", 30, 1000, 1000, false));
    EXPECT_EQ(PwmManager::PWM_ERROR_INVALID_PERIOD_STEPS, manager.registerPwmOutput("ZERO", 2, 0, 1000, false));
    EXPECT_EQ(PwmManager::PWM_ERROR_INVALID_CLOCK_DIVIDER, manager.registerPwmOutput("FAST", 2, 1000000, 1000, false));
    EXPECT_EQ(PwmManager::PWM_ERROR_INVALID_CLOCK_DIVIDER, manager.registerPwmOutput("SLOW", 2, 1, 1000, false));
    EXPECT_EQ(PwmManager::PWM_ERROR_NOT_FOUND, manager.updatePwmOutput("MISSING", 2, 1000, 1000, false));
}

void testCapacityDoesNotBlockUpdates() {
    PwmManager manager;
    const char* names[] = {"PWM0", "PWM1", "PWM2", "PWM3", "PWM4", "PWM5", "PWM6", "PWM7"};
    for (uint gpio = 0; gpio < PwmManager::MAX_PWM_OUTPUTS; ++gpio) {
        EXPECT_EQ(PwmManager::PWM_OK, manager.registerPwmOutput(names[gpio], gpio, 1000, 1000, false));
    }
    EXPECT_TRUE(manager.isFull());
    EXPECT_EQ(PwmManager::PWM_ERROR_REGISTRY_FULL,
        manager.registerPwmOutput("EXTRA", 8, 1000, 1000, false));
    EXPECT_EQ(PwmManager::PWM_OK, manager.updatePwmOutput("PWM0", 0, 1000, 1000, false));

    PwmManager::PwmOutputConfig config{};
    EXPECT_EQ(PwmManager::PWM_OK, manager.getPwmConfigByName("PWM0", config));
    EXPECT_EQ(1000, config.frequencyHz);
    EXPECT_EQ(1000, config.periodSteps);
}

void testCorrectedAttemptClearsHistoricalFailure() {
    PwmManager manager;
    EXPECT_EQ(PwmManager::PWM_ERROR_GPIO_INVALID, manager.registerPwmOutput("STATUS", 30, 1000, 1000, false));
    EXPECT_FALSE(manager.validateRegistrationStatus());
    EXPECT_EQ(PwmManager::PWM_OK, manager.registerPwmOutput("STATUS", 2, 1000, 1000, false));
    EXPECT_TRUE(manager.validateRegistrationStatus());
}
}

int main() {
    testCreateUpdateAndAutomaticRefresh();
    testConflictsAndFailedUpdatePreservesConfiguration();
    testInputAndClockDividerValidation();
    testCapacityDoesNotBlockUpdates();
    testCorrectedAttemptClearsHistoricalFailure();
    if (failures != 0) {
        std::printf("%d PWM test(s) failed.\n", failures);
        return 1;
    }
    std::printf("All PWM tests passed.\n");
    return 0;
}
