cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(NeoPixelPicoProject C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

pico_sdk_init()

add_library(pwm_manager STATIC
    ../src/srcPwm/PwmManager.cpp
    ../src/srcPwm/PwmOutput.cpp
)

target_include_directories(pwm_manager PUBLIC
    ../lib/objPwm
)

target_link_libraries(pwm_manager PUBLIC
    pico_stdlib
    hardware_pwm
    hardware_clocks
)

add_executable(pico_neopixel_objeto
    main.cpp
    ../src/srcNeopixel/NeoPixelLed.cpp
    ../src/srcNeopixel/colores/ColorPalette.cpp
)

target_include_directories(pico_neopixel_objeto PRIVATE
    ../lib/objNeopixel
)

pico_generate_pio_header(pico_neopixel_objeto
    ${CMAKE_CURRENT_LIST_DIR}/../src/srcNeopixel/ws2812.pio
)

target_link_libraries(pico_neopixel_objeto
    pico_stdlib
    hardware_pio
    pwm_manager
)

pico_add_extra_outputs(pico_neopixel_objeto)
