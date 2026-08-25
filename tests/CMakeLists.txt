cmake_minimum_required(VERSION 3.13)
project(PwmHostTests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

add_executable(pwm_tests
    PwmTests.cpp
    PwmTestHardware.cpp
    ../src/srcPwm/PwmManager.cpp
    ../src/srcPwm/PwmOutput.cpp
)

target_include_directories(pwm_tests PRIVATE
    stubs
    ../lib/objPwm
)

target_compile_options(pwm_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME pwm_tests COMMAND pwm_tests)

# Comprueba que el ejemplo público siga compilando contra la API de la librería.
add_library(pwm_instance_example OBJECT
    ../InstanceExamples/InstancePWM_main.cpp
)
target_include_directories(pwm_instance_example PRIVATE
    stubs
    ../lib/objPwm
)
target_compile_options(pwm_instance_example PRIVATE -Wall -Wextra -Wpedantic -Werror)
