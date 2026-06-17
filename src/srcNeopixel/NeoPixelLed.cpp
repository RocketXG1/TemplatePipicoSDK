#include "NeoPixelLed.h"

#include <algorithm>
#include <iterator>

#include "ws2812.pio.h"

namespace {
constexpr float WS2812_FREQUENCY = 800000.0f;
constexpr uint MIN_TRANSITION_STEPS = 1;
constexpr uint MAX_TRANSITION_STEPS = 100;
constexpr uint TARGET_FRAME_TIME_MS = 20;
}

NeoPixelLed::NeoPixelLed(
    uint gpio_pin,
    uint numberOfLeds,
    PIO selectedPioBlock,
    uint selectedStateMachine,
    bool rgbw
) :
    dataPin(gpio_pin),
    ledCount(std::min(numberOfLeds, MAX_LED_COUNT)),
    pioBlock(selectedPioBlock),
    stateMachine(selectedStateMachine),
    programOffset(0),
    isRgbw(rgbw),
    ledBuffer{0} {
}

void NeoPixelLed::init() {
    loadPioProgramIfNeeded();
    ws2812_program_init(pioBlock, stateMachine, programOffset, dataPin, WS2812_FREQUENCY, isRgbw);
    clear();
}

void NeoPixelLed::show() {
    for (uint index = 0; index < ledCount; ++index) {
        sendPixel(ledBuffer[index]);
    }
    sleep_us(80);
}

void NeoPixelLed::clear() {
    fill(Colors::Off);
}

void NeoPixelLed::setPixel(uint index, uint8_t red, uint8_t green, uint8_t blue) {
    if (index >= ledCount) {
        return;
    }

    ledBuffer[index] = colorToGRB(red, green, blue);
}

void NeoPixelLed::setPixel(uint index, const RgbColor& color) {
    setPixel(index, color.red, color.green, color.blue);
}

void NeoPixelLed::fill(uint8_t red, uint8_t green, uint8_t blue) {
    const uint32_t color = colorToGRB(red, green, blue);
    for (uint index = 0; index < ledCount; ++index) {
        ledBuffer[index] = color;
    }
    show();
}

void NeoPixelLed::fill(const RgbColor& color) {
    fill(color.red, color.green, color.blue);
}

uint NeoPixelLed::getLedCount() const {
    return ledCount;
}

void NeoPixelLed::fbLedTransition(
    std::initializer_list<LedSectionTransition> sections,
    uint transitionTimeMs
) {
    const uint steps = std::max(
        MIN_TRANSITION_STEPS,
        std::min(MAX_TRANSITION_STEPS, transitionTimeMs / TARGET_FRAME_TIME_MS)
    );
    const uint delayPerStepMs = steps > 0 ? transitionTimeMs / steps : 0;

    for (uint step = 0; step <= steps; ++step) {
        const uint8_t intensity = static_cast<uint8_t>((255u * step) / steps);
        std::fill(std::begin(ledBuffer), std::begin(ledBuffer) + ledCount, colorToGRB(0, 0, 0));

        uint currentLed = 0;
        for (const LedSectionTransition& section : sections) {
            if (currentLed >= ledCount) {
                break;
            }
            if (section.ledQuantity == 0 || section.colorSequence.size() == 0) {
                continue;
            }

            const uint remainingLeds = ledCount - currentLed;
            const uint sectionLength = std::min(section.ledQuantity, remainingLeds);
            setSectionColor(currentLed, sectionLength, section.colorSequence, intensity);
            currentLed += sectionLength;
        }

        show();
        if (delayPerStepMs > 0 && step < steps) {
            sleep_ms(delayPerStepMs);
        }
    }
}

uint32_t NeoPixelLed::colorToGRB(uint8_t red, uint8_t green, uint8_t blue) const {
    uint32_t grb = (static_cast<uint32_t>(green) << 16) |
                   (static_cast<uint32_t>(red) << 8) |
                   static_cast<uint32_t>(blue);

    if (isRgbw) {
        grb <<= 8;
    }

    return grb;
}

void NeoPixelLed::sendPixel(uint32_t pixelColor) const {
    if (isRgbw) {
        pio_sm_put_blocking(pioBlock, stateMachine, pixelColor);
    } else {
        pio_sm_put_blocking(pioBlock, stateMachine, pixelColor << 8u);
    }
}

void NeoPixelLed::loadPioProgramIfNeeded() {
    if (!pio_can_add_program(pioBlock, &ws2812_program)) {
        return;
    }

    programOffset = pio_add_program(pioBlock, &ws2812_program);
}

RgbColor NeoPixelLed::scaleColor(const RgbColor& color, uint8_t intensity) const {
    return {
        static_cast<uint8_t>((static_cast<uint16_t>(color.red) * intensity) / 255u),
        static_cast<uint8_t>((static_cast<uint16_t>(color.green) * intensity) / 255u),
        static_cast<uint8_t>((static_cast<uint16_t>(color.blue) * intensity) / 255u)
    };
}

void NeoPixelLed::setSectionColor(
    uint startIndex,
    uint sectionLength,
    std::initializer_list<RgbColor> colors,
    uint8_t intensity
) {
    if (startIndex >= ledCount || sectionLength == 0 || colors.size() == 0) {
        return;
    }

    const uint safeLength = std::min(sectionLength, ledCount - startIndex);
    for (uint offset = 0; offset < safeLength; ++offset) {
        const uint colorIndex = (offset * colors.size()) / safeLength;
        const RgbColor* color = colors.begin() + std::min<uint>(colorIndex, colors.size() - 1);
        setPixel(startIndex + offset, scaleColor(*color, intensity));
    }
}
