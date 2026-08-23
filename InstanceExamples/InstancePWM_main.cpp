#include "PwmManager.h"
#include "PwmOutput.h"

#include <cstdio>

#include "pico/stdlib.h"

int main() {
    stdio_init_all();
    sleep_ms(2000);

    PwmManager pwmManager;

    // Salida independiente para el control de un spindle.
    pwmManager.registerPwmOutput(
        "SPINDLE",
        2,
        20000,
        1000,
        true
    );

    // LIGHT y FAN usan los dos canales del mismo slice. Por ello deben tener
    // exactamente la misma frecuencia y la misma cantidad de periodSteps.
    pwmManager.registerPwmOutput(
        "LIGHT",
        4,
        1000,
        1000,
        false
    );
    pwmManager.registerPwmOutput(
        "FAN",
        5,
        1000,
        1000,
        false
    );

    // Salida independiente de menor frecuencia para una bomba.
    pwmManager.registerPwmOutput(
        "PUMP",
        6,
        500,
        2000,
        true
    );

    // Un servomotor de 180 grados utiliza una señal típica de 50 Hz.
    pwmManager.registerPwmOutput(
        "SERVO180",
        8,
        50,
        20000,
        true
    );

    std::printf("Mapa PWM antes de actualizar PUMP:\n");
    pwmManager.printPwmMap();

    // registerPwmOutput detecta automáticamente que PUMP ya existe por su NAME.
    // Antes de sobrescribirlo, valida estos valores contra los demás PWM.
    PwmManager::PwmRegistrationAction pumpAction =
        PwmManager::PwmRegistrationAction::None;
    const int pumpUpdateResult = pwmManager.registerPwmOutput(
        "PUMP",
        6,
        750,
        1000,
        true,
        &pumpAction
    );
    if (pumpUpdateResult != PwmManager::PWM_OK) {
        std::printf("No fue posible actualizar PUMP. Código: %d\n", pumpUpdateResult);
        while (true) {
            tight_loop_contents();
        }
    }
    if (pumpAction == PwmManager::PwmRegistrationAction::Updated) {
        std::printf("PUMP ya existía y fue actualizado.\n");
    } else if (pumpAction == PwmManager::PwmRegistrationAction::Registered) {
        std::printf("PUMP no existía y fue registrado.\n");
    }

    std::printf("Mapa PWM después de actualizar PUMP:\n");
    pwmManager.printPwmMap();
    if (!pwmManager.validateRegistrationStatus()) {
        std::printf("No fue posible registrar todos los PWM.\n");
        while (true) {
            tight_loop_contents();
        }
    }

    PwmOutput spindle(pwmManager);
    PwmOutput light(pwmManager);
    PwmOutput fan(pwmManager);
    PwmOutput pump(pwmManager);
    PwmOutput servo180(pwmManager);

    if (
        spindle.init("SPINDLE") != PwmManager::PWM_OK ||
        light.init("LIGHT") != PwmManager::PWM_OK ||
        fan.init("FAN") != PwmManager::PWM_OK ||
        pump.init("PUMP") != PwmManager::PWM_OK ||
        servo180.init("SERVO180") != PwmManager::PWM_OK
    ) {
        std::printf("No fue posible inicializar todos los PWM.\n");
        while (true) {
            tight_loop_contents();
        }
    }

    while (true) {
        spindle.setDutyPercent(75.0f);
        light.setDutyPercent(35.0f);
        fan.setDutyPercent(60.0f);
        pump.setDutyPercent(50.0f);

        // Mueve el servomotor entre sus posiciones mínima, central y máxima.
        servo180.setServoAngle180(0.0f);
        sleep_ms(1000);
        servo180.setServoAngle180(90.0f);
        sleep_ms(1000);
        servo180.setServoAngle180(180.0f);
        sleep_ms(1000);
    }
}
