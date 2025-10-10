#include "neopixel_control.h"
#include "pins.h"

static Adafruit_NeoPixel *strip = nullptr;
static uint8_t current_brightness = 128;

void neopixel_init() {
    if (strip) return;
    // If NEOPIXEL_GND_PIN is defined, drive it low to serve as a ground reference.
#ifdef NEOPIXEL_GND_PIN
    pinMode(NEOPIXEL_GND_PIN, OUTPUT);
    digitalWrite(NEOPIXEL_GND_PIN, LOW);
#endif
    strip = new Adafruit_NeoPixel(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
    strip->begin();
    strip->setBrightness(current_brightness);
    strip->show();
}

void neopixel_ground_enable() {
#ifdef NEOPIXEL_GND_PIN
    pinMode(NEOPIXEL_GND_PIN, OUTPUT);
    digitalWrite(NEOPIXEL_GND_PIN, LOW);
#endif
}

void neopixel_ground_disable() {
#ifdef NEOPIXEL_GND_PIN
    // To 'disable' the software ground, set pin to HIGH-Z
    pinMode(NEOPIXEL_GND_PIN, INPUT);
#endif
}

void neopixel_set_color(uint8_t r, uint8_t g, uint8_t b) {
    if (!strip) neopixel_init();
    for (uint16_t i = 0; i < strip->numPixels(); i++) {
        strip->setPixelColor(i, strip->Color(r, g, b));
    }
    strip->show();
}

void neopixel_set_pixel(uint16_t idx, uint8_t r, uint8_t g, uint8_t b) {
    if (!strip) neopixel_init();
    if (idx >= strip->numPixels()) return;
    strip->setPixelColor(idx, strip->Color(r, g, b));
    strip->show();
}

void neopixel_set_brightness(uint8_t b) {
    if (!strip) neopixel_init();
    current_brightness = b;
    strip->setBrightness(current_brightness);
    strip->show();
}

void neopixel_clear() {
    if (!strip) neopixel_init();
    strip->clear();
    strip->show();
}

Adafruit_NeoPixel* neopixel_get() {
    if (!strip) neopixel_init();
    return strip;
}
