#ifndef NEOPIXEL_LED_H
#define NEOPIXEL_LED_H

#include <cstdint>
#include <initializer_list>

#include "hardware/pio.h"
#include "pico/stdlib.h"

#include "colores/ColorPalette.h"

struct LedSectionTransition {
    uint ledQuantity;
    std::initializer_list<RgbColor> colorSequence;
};

class NeoPixelLed {
public:
    static constexpr uint MAX_LED_COUNT = 150;

    NeoPixelLed(
        uint gpio_pin,
        uint numberOfLeds,
        PIO selectedPioBlock,
        uint selectedStateMachine,
        bool rgbw = false
    );

    void init();
    void show();
    void clear();
    void setPixel(uint index, uint8_t red, uint8_t green, uint8_t blue);
    void setPixel(uint index, const RgbColor& color);
    void fill(uint8_t red, uint8_t green, uint8_t blue);
    void fill(const RgbColor& color);
    uint getLedCount() const;
    void fbLedTransition(
        std::initializer_list<LedSectionTransition> sections,
        uint transitionTimeMs
    );

private:
    uint dataPin;
    uint ledCount;
    PIO pioBlock;
    uint stateMachine;
    uint programOffset;
    bool isRgbw;
    uint32_t ledBuffer[MAX_LED_COUNT];

    uint32_t colorToGRB(uint8_t red, uint8_t green, uint8_t blue) const;
    void sendPixel(uint32_t pixelColor) const;
    void loadPioProgramIfNeeded();
    RgbColor blendColors(const RgbColor& startColor, const RgbColor& endColor, uint step, uint totalSteps) const;
    RgbColor getSequenceColor(std::initializer_list<RgbColor> colors, uint step, uint totalSteps) const;
    void setSectionColor(
        uint startIndex,
        uint sectionLength,
        std::initializer_list<RgbColor> colors,
        uint step,
        uint totalSteps
    );
};

#endif
