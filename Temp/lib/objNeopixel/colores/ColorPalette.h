#ifndef COLOR_PALETTE_H
#define COLOR_PALETTE_H

#include <cstdint>

struct RgbColor {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

namespace Colors {
extern const RgbColor Red;
extern const RgbColor Green;
extern const RgbColor Blue;
extern const RgbColor White;
extern const RgbColor Off;
extern const RgbColor Yellow;
extern const RgbColor Cyan;
extern const RgbColor Magenta;
extern const RgbColor Orange;
extern const RgbColor Brown;
extern const RgbColor Violet;
}

#endif
