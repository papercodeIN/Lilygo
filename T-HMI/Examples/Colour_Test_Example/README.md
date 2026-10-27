# Colour Test Example

This example checks the LILYGO T-HMI display by showing its logo, colored text, and repeated display-inversion patterns. It is useful for verifying basic screen initialization and colors.

## Project files

- `Colour_Test_Example.ino` — Main sketch; initializes the display and draws test screens.
- `pins.h` — T-HMI board pin definitions.
- `logo.h` and `logo.jpg` — Logo image data/source asset.

## Requirements

- LILYGO T-HMI ESP32-S3 board and USB data cable.
- Visual Studio Code with the **Arduino** extension and Arduino CLI/IDE, or Arduino IDE directly.
- ESP32 Arduino board support compatible with the T-HMI.
- `TFT_eSPI`, configured for the T-HMI display and matching `pins.h`.

This folder does not currently contain a `platformio.ini`; it is an Arduino sketch, not yet a standalone PlatformIO project. PlatformIO cannot build it until a project configuration and display-library configuration are supplied.

## Open and upload with VS Code

1. Install VS Code and the Microsoft **Arduino** extension. Install Arduino IDE or Arduino CLI and configure ESP32 board support first.
2. Open this folder in VS Code (**File > Open Folder...**).
3. Install and configure `TFT_eSPI` for the T-HMI. Use the board's correct display configuration rather than a generic TFT setup.
4. Select the ESP32-S3 board and the port for the connected T-HMI. If prompted, set the sketch folder to this folder.
5. Verify/compile, then upload over USB. Open the serial monitor at **115200 baud** if you want to see initialization messages.

Alternatively, open `Colour_Test_Example.ino` in Arduino IDE, select the matching board and port, and use **Verify** followed by **Upload**.

## What you should see

The sketch displays a startup logo, then cycles through colored text and inverted/non-inverted display tests. The test screens pause briefly before changing.

## Customize and troubleshoot

- Change the text, colors, or delay in `Colour_Test_Example.ino`.
- If the logo is missing or garbled, check that `logo.h` is present and that image dimensions/data format have not been changed independently.
- If the screen is blank or colors are wrong, check the T-HMI TFT_eSPI setup and `pins.h`.
- If VS Code cannot locate a library, confirm that it is installed in the Arduino environment selected by the extension.
- If upload cannot connect, hold **BOOT**, press and release **RESET**, release **BOOT**, then retry.

For common setup and safety notes, see [`../README.md`](../README.md).
