#include "NeoPixelLed.h"
#include "colores/ColorPalette.h"

#include "pico/stdlib.h"

int main() {
    stdio_init_all();

    NeoPixelLed statusStrip(
        16,
        1,
        pio0,
        0,
        false
    );

    NeoPixelLed LedStrip(
        29,
        100,
        pio0,
        1,
        false
    );

    statusStrip.init();
    LedStrip.init();
   while (true) {
         statusStrip.setPixel(0,Colors::Cyan);
         statusStrip.show();
          

          
        
        LedStrip.fbLedTransition(
            {
                {
                    25,
                    {
                        Colors::Red,
                        Colors::Blue,
                        Colors::Magenta,
                        Colors::Green
                    }
                },
                {
                    25,
                    {
                        Colors::Blue,
                        Colors::Red,
                        Colors::Orange
                    }
                },
                {
                    25,
                    {
                        Colors::Red,
                        Colors::Green
                    }
                },
                {
                    25,
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
            500
        );

        sleep_ms(500);
        //LedStrip.clear();
        

        LedStrip.fbLedTransition(
            {
                {80, {Colors::Cyan, Colors::White}},
                {10, {Colors::Violet, Colors::Magenta, Colors::Blue}},
                {10, {Colors::Orange, Colors::Brown, Colors::Yellow}}
            },
            5000
        );

        sleep_ms(750);
    }    
}
