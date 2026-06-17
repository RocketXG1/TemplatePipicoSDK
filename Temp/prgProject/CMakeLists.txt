cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(NeoPixelPicoProject C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(NeoPixelPicoProject
    main.cpp
    ../src/srcNeopixel/NeoPixelLed.cpp
    ../src/srcNeopixel/colores/ColorPalette.cpp
)

target_include_directories(NeoPixelPicoProject PRIVATE
    ../lib/objNeopixel
)

pico_generate_pio_header(NeoPixelPicoProject
    ${CMAKE_CURRENT_LIST_DIR}/../src/srcNeopixel/ws2812.pio
)

target_link_libraries(NeoPixelPicoProject
    pico_stdlib
    hardware_pio
)

pico_add_extra_outputs(NeoPixelPicoProject)
