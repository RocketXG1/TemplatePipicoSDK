#include "NeoPixelLed.h"
#include "colores/ColorPalette.h"

#include "pico/stdlib.h"

int main() {
    stdio_init_all();

    NeoPixelLed statusStrip(
        16,
        34,
        pio0,
        0,
        false
    );

    statusStrip.init();

    while (true) {
        statusStrip.fbLedTransition(
            {
                {
                    4,
                    {
                        Colors::Red,
                        Colors::Blue,
                        Colors::Magenta,
                        Colors::Green
                    }
                },
                {
                    8,
                    {
                        Colors::Blue,
                        Colors::Red,
                        Colors::Orange
                    }
                },
                {
                    2,
                    {
                        Colors::Red,
                        Colors::Green
                    }
                },
                {
                    20,
                    {
                        Colors::Red,
                        Colors::Red,
                        Colors::Green,
                        Colors::Orange,
                        Colors::Yellow,
                        Colors::Blue
                    }
                }
            },
            1000
        );

        sleep_ms(500);
        statusStrip.clear();
        sleep_ms(500);

        statusStrip.fbLedTransition(
            {
                {10, {Colors::Cyan, Colors::White}},
                {12, {Colors::Violet, Colors::Magenta, Colors::Blue}},
                {12, {Colors::Orange, Colors::Brown, Colors::Yellow}}
            },
            1200
        );

        sleep_ms(750);
    }
}
