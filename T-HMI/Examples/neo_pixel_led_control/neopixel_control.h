#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

void neopixel_init();
void neopixel_set_color(uint8_t r, uint8_t g, uint8_t b);
void neopixel_set_brightness(uint8_t b);
void neopixel_clear();

// Control a GPIO used as a software ground for the NeoPixel strip.
// WARNING: Driving ground with a GPIO can only source/sink very small currents
// (tens of mA). Prefer a true GND connection or a MOSFET-based switch for
// powering many LEDs. Use these functions only if you understand the hardware.
void neopixel_ground_enable();
void neopixel_ground_disable();

// helper to set a single pixel (index, 0-based)
void neopixel_set_pixel(uint16_t idx, uint8_t r, uint8_t g, uint8_t b);

// return pointer to Adafruit_NeoPixel instance (for advanced users)
Adafruit_NeoPixel* neopixel_get();
