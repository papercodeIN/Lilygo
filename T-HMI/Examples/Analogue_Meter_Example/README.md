# Analogue Meter Example

This example displays two animated gauges labelled **Amps** and **Volts** on the LILYGO T-HMI screen. The readings are generated test values (a changing sine wave); they are not measurements from real sensors.

## Project files

- `Analogue_Meter_Example.ino` — Arduino sketch and gauge setup/update loop.
- `pins.h` — T-HMI board pin definitions.

## Requirements

- LILYGO T-HMI ESP32-S3 board and USB data cable.
- Visual Studio Code with the **Arduino** extension and Arduino CLI/IDE, or use Arduino IDE directly.
- ESP32 Arduino board support compatible with the T-HMI.
- `TFT_eSPI` and its `TFT_eWidget` gauge widget library, installed in the Arduino library environment.
- TFT_eSPI configuration suitable for the T-HMI display and matching `pins.h`.

This folder does not currently contain a `platformio.ini`; it is an Arduino sketch, not yet a standalone PlatformIO project. PlatformIO cannot build it until a project configuration and its library/display configuration are supplied.

## Open and upload with VS Code

1. Install VS Code and the Microsoft **Arduino** extension. Install Arduino IDE or Arduino CLI and configure ESP32 board support first.
2. Open this folder in VS Code (**File > Open Folder...**).
3. Install `TFT_eSPI` and `TFT_eWidget` into the same Arduino environment VS Code uses. Make sure TFT_eSPI is configured for the T-HMI; a generic display configuration may compile but fail to display correctly.
4. Select the ESP32-S3 board and serial port using the Arduino extension's board/port selectors. If prompted, set the sketch folder to this folder.
5. Verify/compile, connect the board over USB, and upload. Open the serial monitor at **115200 baud** if needed.

To use Arduino IDE instead, open `Analogue_Meter_Example.ino` from this folder, configure the same board and libraries, then verify and upload.

## What you should see

Two circular meters appear on the display. Their needles move continuously to demonstrate the gauge widgets. The sketch updates about every 35 ms and maps the generated value to 0–3 A and 0–12 V display ranges.

## Customize and troubleshoot

- Edit the gauge ranges, labels, and zones in `Analogue_Meter_Example.ino`.
- Replace the generated sine-wave value with sensor readings only after adding and validating the appropriate sensor hardware and code.
- If the screen is blank or has incorrect colors, check the TFT_eSPI T-HMI configuration and pin mapping before changing gauge code.
- If VS Code cannot find libraries, confirm they are installed for the Arduino CLI/IDE instance selected by the extension.
- If upload cannot connect, hold **BOOT**, press and release **RESET**, release **BOOT**, then retry.

For common setup and safety notes, see [`../README.md`](../README.md).
