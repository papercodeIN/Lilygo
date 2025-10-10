# T-HMI LED Control
This project provides firmware and code examples for controlling LEDs using the T-HMI board. It includes source files for hardware initialization and LED control logic.

## Features
- LED control via T-HMI board
- Image and font assets for UI

## Folder Structure
- `t-hmi_led_control.ino` — Main Arduino sketch
- `data.cpp`, `data.h` — Data handling logic
- `init_code.h`, `pins.h`, `lv_conf.h` — Hardware and LVGL configuration
- `image/` — Logo and image assets

## Usage
- After uploading, the T-HMI board will run the LED control functionality.
- Modify `t-hmi_led_control.ino` to customize LED behavior.

## NeoPixel (WS2812B) Support

Wiring:
- Connect NeoPixel data-in to the pin defined by `NEOPIXEL_PIN` in `pins.h` (default 16).
- Connect NeoPixel power (5V) and GND to the same ground as the T-HMI board. If using many LEDs, power the strip from a separate 5V supply and common ground.

Note for your wiring:
- This example was updated to use pin 15 as the NeoPixel data pin (`NEOPIXEL_PIN = 15`) and pin 16 as the NeoPixel ground (`NEOPIXEL_GND_PIN = 16`) per your request.

NOTE about `NEOPIXEL_GND_PIN`:
- The project now defines `NEOPIXEL_GND_PIN` (pin 16) and the firmware will drive this pin LOW during `neopixel_init()` to act as a software ground if you wire the strip ground to it.
- WARNING: Most microcontroller GPIO pins cannot sink the large currents required by NeoPixel strips. Driving ground from a GPIO is only safe for a very small number of LEDs (tens of mA). For anything beyond a couple of LEDs, wire the strip ground directly to the board ground or use a MOSFET/transistor switch controlled by a GPIO to gate the 5V power.

If you prefer not to use a GPIO as ground, remove `NEOPIXEL_GND_PIN` from `pins.h` and wire the strip ground to the board ground instead. If you want me to implement a proper PWR enable (MOSFET-based) control using an available pin, I can add that.
- If needed, add a 300-500 Ohm resistor in series with the data line and a 1000 uF capacitor across 5V and GND near the strip.

Usage:
- The UI includes a "NeoPixel Control" panel with sliders for R, G, B and Brightness and an Apply button. Adjust values and press Apply to update the strip.

Library:
- This example uses the Adafruit_NeoPixel library. Install it through the Arduino Library Manager (Adafruit NeoPixel).
