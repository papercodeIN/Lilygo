<div align="center">

<img src="https://visitor-badge.laobi.icu/badge?page_id=papercodeIN.Lilygo.T-HMI.NeoPixel_LED_Control" alt="Example visitors" height="20" />

<p align="center">
  <strong>Support</strong><br>
  If these projects help you, connect with us on social media:
</p>


[![YouTube](https://img.shields.io/badge/YouTube-Fusion_Automate-FF0000?logo=youtube&logoColor=white)](https://www.youtube.com/channel/UCKKhdFV0q8CV5vWUDfiDfTw)
[![Telegram](https://img.shields.io/badge/Telegram-%40fusionautomate-26A5E4?logo=telegram&logoColor=white)](https://t.me/fusionautomate)
[![Instagram](https://img.shields.io/badge/Instagram-%40fusion__automate-E4405F?logo=instagram&logoColor=white)](https://www.instagram.com/fusion_automate)
[![WhatsApp](https://img.shields.io/badge/WhatsApp-%2B91--9974477759-25D366?logo=whatsapp&logoColor=white)](https://wa.me/919974477759)
[![Buy Me a Coffee](https://img.shields.io/badge/Buy_Me_a_Coffee-support-FFDD00?logo=buymeacoffee&logoColor=black)](https://buymeacoffee.com/pylin)
</div>

# NeoPixel LED Control

This LVGL touchscreen example controls a WS2812B/NeoPixel strip connected to the LILYGO T-HMI. The screen provides RGB and brightness controls and an Apply action.

## Project files

- `neo_pixel_led_control.ino` — Main sketch and display/touch initialization.
- `neopixel_control.h` / `neopixel_control.cpp` — NeoPixel operations and user-interface integration.
- `pins.h` — T-HMI and NeoPixel pin definitions.
- `init_code.h`, `lv_conf.h` — LCD initialization and LVGL configuration.
- `data.h` / `data.cpp` — Touch calibration storage support.
- `image/` — Logo assets.

## Hardware and safety

- LILYGO T-HMI ESP32-S3 board and USB data cable.
- Compatible WS2812B/NeoPixel LEDs.
- The current `pins.h` sets `NEOPIXEL_PIN` to GPIO 15, `NEOPIXEL_GND_PIN` to GPIO 16, and `NEOPIXEL_COUNT` to 2. Check your board and wiring before connecting LEDs. The sketch configures `NEOPIXEL_GND_PIN` as an output and drives it LOW; do not connect an external strip's ground to this GPIO because it is not rated as a power return.
- Power a strip from an appropriately rated external 5 V supply when needed, and connect its ground to board ground. GPIOs are not power grounds and must not carry strip current; do not use GPIO 16 as a ground for an external strip. Use a proper ground connection and, if switched power is required, a suitably rated transistor/MOSFET circuit.
- A series data resistor and a capacitor across strip 5 V/GND near the strip are commonly used to protect signal integrity and reduce supply transients.

## Software requirements

- VS Code with the **PlatformIO IDE** extension, or Arduino IDE/CLI.
- ESP32 Arduino core compatible with the T-HMI panel APIs.
- LVGL 8.x, the XPT2046 touch driver, and Adafruit NeoPixel library, plus the T-HMI support libraries/headers required by the sketch.

This folder currently has no `platformio.ini`, so it is not a standalone PlatformIO project. PlatformIO needs its own project configuration and declared dependencies before it can build this example.

## Build and upload

### VS Code / PlatformIO

1. Install VS Code and the PlatformIO IDE extension.
2. Open this example folder in VS Code. PlatformIO will not identify it as a buildable project until a `platformio.ini` is added.
3. To enable PlatformIO builds, add a configuration for the T-HMI-compatible ESP32-S3 target and declare LVGL, XPT2046, Adafruit NeoPixel, and the required local T-HMI support files.

### Arduino IDE

1. Install Arduino IDE and ESP32 board support compatible with this example's LCD panel APIs.
2. Install the required libraries into the selected Arduino environment and configure the board-specific T-HMI support.
3. Open `neo_pixel_led_control.ino`, select the appropriate board and port, compile, and upload.
4. If upload cannot connect, hold **BOOT**, press and release **RESET**, release **BOOT**, then retry.

## Operation and customization

After startup, use the on-screen RGB and brightness controls and press **Apply** to update the configured LEDs. Change `NEOPIXEL_COUNT` to match the strip, and change `NEOPIXEL_PIN` only when the wiring and available GPIO support it. Keep LED power and GPIO signal wiring separate; use a common ground between the external supply and T-HMI.

## Troubleshooting

- **LEDs stay dark:** check external 5 V supply, common ground, data direction, data pin, and LED count.
- **Wrong colors:** verify the strip's color order and the library configuration.
- **Display or touch fails:** verify the T-HMI-compatible ESP32 core, panel libraries, `pins.h`, and touch calibration.
- **Build cannot find a library:** install it into the Arduino/PlatformIO environment actually selected by the editor and ensure the required project configuration exists.

For common setup and safety notes, see [`../README.md`](../README.md).
