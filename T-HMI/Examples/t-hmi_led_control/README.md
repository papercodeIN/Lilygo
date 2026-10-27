# T-HMI Touchscreen LED Control

This example uses the T-HMI touchscreen to control the LED connected to the board's LED GPIO. The LVGL screen provides short blink, long blink, blinking-pattern, and counter-reset actions.

## Project files

- `t-hmi_led_control.ino` — Main sketch, display/touch initialization, and LVGL controls.
- `pins.h` — T-HMI GPIO definitions, including `LED_PIN`.
- `init_code.h` — LCD initialization sequence.
- `lv_conf.h` — LVGL configuration used by the sketch.
- `data.h` / `data.cpp` — Touch calibration storage support.
- `image/` — Startup logo assets.

## Requirements

- LILYGO T-HMI ESP32-S3 board and USB data cable.
- Visual Studio Code and **PlatformIO IDE** extension, or Arduino IDE/CLI.
- ESP32 Arduino core and the T-HMI-compatible ESP-IDF LCD panel headers/APIs.
- LVGL 8.x, the XPT2046 touch driver, and other T-HMI support libraries used by the sketch.

This folder currently has no `platformio.ini`, so it is not a standalone PlatformIO project yet. For Arduino IDE, required libraries and board-specific support must be installed in the selected Arduino environment. Do not treat this as a ready-to-build PlatformIO project until a project configuration and its dependencies are added.

## Build with VS Code and PlatformIO

1. Install VS Code and the **PlatformIO IDE** extension.
2. Open the repository folder in VS Code. PlatformIO requires a `platformio.ini` to identify a project; this example folder does not currently provide one, so PlatformIO build/upload is not available for it yet.
3. To build with PlatformIO, first add/restore a project configuration that selects the T-HMI-compatible ESP32-S3 board/core, supplies LVGL and XPT2046 dependencies, and makes all required headers/libraries available. Keep the configuration and project sources together and document the settings.

## Build with Arduino IDE

1. Install Arduino IDE and ESP32 board support compatible with the T-HMI panel APIs used by this example.
2. Install/configure LVGL 8.x, XPT2046, and the T-HMI support files required by the repository.
3. Open `t-hmi_led_control.ino` in this folder and select the matching ESP32-S3 board, USB options, PSRAM, and serial port. Board-menu names vary with the installed core version; use the repository's T-HMI setup instructions.
4. Compile, connect the board via USB, and upload. If upload cannot connect, hold **BOOT**, press and release **RESET**, release **BOOT**, then retry.

## Use and customize

After boot, use the touchscreen buttons to trigger LED behavior. The counter displays button actions and can be reset from the interface. To customize the controls or behavior, edit the handlers and UI creation code in `t-hmi_led_control.ino`; change `LED_PIN` in `pins.h` only if your hardware wiring differs.

## Troubleshooting

- **Build errors for LVGL or XPT2046:** install the correct library versions and ensure the compiler can find the project headers.
- **Blank display:** check ESP32 core compatibility, LCD panel support, `init_code.h`, and `pins.h`.
- **Touch does not respond accurately:** check the touch wiring and calibration data support.
- **LED does not blink:** confirm the board LED GPIO and `LED_PIN`; do not connect external loads directly to a GPIO.
- **Upload fails:** close the serial monitor, verify the selected port, and enter download mode with BOOT/RESET.

For common setup and safety notes, see [`../README.md`](../README.md).
