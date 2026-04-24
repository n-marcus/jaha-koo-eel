#include <Adafruit_NeoPixel.h>

#define PIN D9
#define NUMPIXELS 3

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup()
{
    pixels.begin();
}

void loop()
{
    pixels.fill(0, pixels.Color(255, 0, 0));
    pixels.show();
    delay(500);

    pixels.fill(pixels.Color(0, 255, 0));
    pixels.show();
    delay(500);

    pixels.fill(pixels.Color(0, 0, 255));
    pixels.show();
    delay(500);
}
