# T-HMI Examples

This folder contains example programs for the LILYGO T-HMI ESP32-S3 touchscreen board. The projects include display tests, LVGL touch interfaces, LED control, and Modbus TCP relay control.

## Projects

| Folder | What it demonstrates | Project status |
| --- | --- | --- |
| `Analogue_Meter_Example` | Two animated analog gauges using TFT_eSPI widgets | Arduino sketch; no project-local `platformio.ini` yet |
| `Colour_Test_Example` | Display colors, text, logo, and inversion | Arduino sketch; no project-local `platformio.ini` yet |
| `mbtcp_coil_control` | LVGL touchscreen control of Modbus TCP relay coils over Wi-Fi | Includes `platformio.ini`; see its README for a board-definition path requirement |
| `neo_pixel_led_control` | LVGL touchscreen control of WS2812B/NeoPixel LEDs | Arduino sketch; no project-local `platformio.ini` yet |
| `t-hmi_led_control` | LVGL touchscreen interface to operate the board's LED | Arduino sketch; no project-local `platformio.ini` yet |

## Before you start

- LILYGO T-HMI ESP32-S3 board and USB data cable.
- A computer with [Visual Studio Code](https://code.visualstudio.com/) installed.
- VS Code extension: **PlatformIO IDE** (install from the Extensions view).
- For Arduino-only examples, VS Code extension: **Arduino** (Microsoft), plus the Arduino CLI or Arduino IDE and ESP32 board support. PlatformIO can also build Arduino sketches once a valid PlatformIO project configuration exists.
- Install any libraries required by the project README. The T-HMI display and touch drivers may require the board-specific libraries and configurations supplied by the upstream T-HMI project.

## Open a project in VS Code

1. Download or clone the repository and keep its folders together. Some PlatformIO configurations use paths relative to the repository root.
2. In VS Code, choose **File > Open Folder...** and open one project folder, such as `Examples/Colour_Test_Example`.
3. For a PlatformIO project, wait for PlatformIO to initialize, then use the PlatformIO toolbar or the PlatformIO sidebar to **Build**, **Upload**, and **Monitor**.
4. For a sketch-only project, open the `.ino` file and follow the project README's Arduino setup steps. The Arduino extension is not a substitute for a missing project configuration or missing library.

## Arduino IDE / CLI essentials

Open the `.ino` inside the folder whose name matches the sketch. Select an ESP32-S3 board configuration appropriate for T-HMI, connect the board, select its serial port, compile, then upload. If using Arduino CLI or a different editor, configure the same board package, libraries, and project settings. Exact board menu values can vary with ESP32 core version; use each project README and the current T-HMI hardware documentation rather than guessing settings.

## Upload mode and serial monitor

If upload cannot connect, hold **BOOT**, press and release **RESET**, then release **BOOT** and retry. Open the serial monitor at the baud rate stated in the project README (many examples use 115200). Close any other program using the serial port before uploading or opening the monitor.

## Build, edit, and contribute

- Start by building and uploading an unmodified example so that hardware and tooling are known to work.
- Make small changes in the sketch or its companion `.h`/`.cpp` files, then build again.
- Keep each sketch with its matching headers, source files, and assets; do not move a single source file away from its project dependencies.
- For projects with `platformio.ini`, build from that project's folder. Do not assume the repository root is a PlatformIO project.
- Record any new hardware wiring, library requirements, board settings, and test results in that project's README.

## Safety

Use a USB data cable and suitable power supply. When connecting external LED strips or relay modules, check voltage, current, common-ground requirements, and GPIO limits. Never power a high-current load from an ESP32 GPIO. Relay projects can switch dangerous voltages; use isolated hardware and follow electrical safety practices.
