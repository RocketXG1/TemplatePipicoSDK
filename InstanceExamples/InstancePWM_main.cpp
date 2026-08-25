#include "PwmManager.h"
#include "PwmOutput.h"

#include <cstdio>

#include "pico/stdlib.h"

namespace {
[[noreturn]] void stopWithError(const char* operation, int errorCode) {
    std::printf("ERROR: %s. Código PWM: %d\n", operation, errorCode);
    while (true) tight_loop_contents();
}

void requirePwmOk(int result, const char* operation) {
    if (result != PwmManager::PWM_OK) stopWithError(operation, result);
}
}

int main() {
    stdio_init_all();
    sleep_ms(2000);

    // El manager debe vivir más tiempo que todos los PwmOutput asociados.
    PwmManager pwmManager;

    // 1. Registrar las configuraciones antes de instanciar el hardware.
    // LIGHT y FAN comparten un slice en los canales A y B, por eso deben usar
    // exactamente la misma frecuencia y periodSteps y no exigir exclusividad.
    requirePwmOk(
        pwmManager.registerPwmOutput("LIGHT", 4, 1000, 1000, false),
        "registrar LIGHT"
    );
    requirePwmOk(
        pwmManager.registerPwmOutput("FAN", 5, 1000, 1000, false),
        "registrar FAN"
    );

    // La bomba y el servo reservan sus slices completos.
    requirePwmOk(
        pwmManager.registerPwmOutput("PUMP", 6, 500, 2000, true),
        "registrar PUMP"
    );
    requirePwmOk(
        pwmManager.registerPwmOutput("SERVO180", 8, 50, 20000, true),
        "registrar SERVO180"
    );

    if (!pwmManager.validateRegistrationStatus()) {
        std::printf("ERROR: el mapa PWM contiene configuraciones inválidas.\n");
        while (true) tight_loop_contents();
    }
    pwmManager.printPwmMap();

    // 2. Instanciar cada salida con el manager y enlazarla con su NAME.
    PwmOutput light(pwmManager);
    PwmOutput fan(pwmManager);
    PwmOutput pump(pwmManager);
    PwmOutput servo180(pwmManager);

    requirePwmOk(light.init("LIGHT"), "inicializar LIGHT");
    requirePwmOk(fan.init("FAN"), "inicializar FAN");
    requirePwmOk(pump.init("PUMP"), "inicializar PUMP");
    requirePwmOk(servo180.init("SERVO180"), "inicializar SERVO180");

    // 3. Una salida ya inicializada se actualiza automáticamente por NAME.
    // PUMP pasa del GPIO 6 al GPIO 10 y cambia su temporización. Si la validación
    // es correcta, updatePwmOutput() actualiza `pump`, limpia el GPIO 6 y configura
    // el GPIO 10 sin volver a llamar pump.init(). Si falla, conserva lo anterior.
    requirePwmOk(
        pwmManager.updatePwmOutput("PUMP", 10, 1000, 1000, true),
        "actualizar PUMP"
    );
    std::printf(
        "PUMP actualizado automáticamente: GPIO=%u, slice=%u, frecuencia=%lu Hz\n",
        pump.getGpioPin(),
        pump.getSliceNum(),
        static_cast<unsigned long>(pump.getFrequencyHz())
    );
    pwmManager.printPwmMap();

    // 4. Controlar las salidas usando unidades adecuadas para cada aplicación.
    while (true) {
        light.setDutyPercent(35.0f);
        fan.setDutyPercent(60.0f);
        pump.setDutyPercent(50.0f);

        servo180.setServoAngle180(0.0f);
        sleep_ms(1000);
        servo180.setServoAngle180(90.0f);
        sleep_ms(1000);
        servo180.setServoAngle180(180.0f);
        sleep_ms(1000);
    }
}
