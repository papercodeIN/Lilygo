<div align="center">

<img src="https://visitor-badge.laobi.icu/badge?page_id=papercodeIN.Lilygo.T-HMI.Examples" alt="Examples page visitors" height="20" />

<p align="center">
  <strong>Support</strong><br>
  If these projects help you, consider supporting the channel:
</p>

<p align="center">
  <a href="https://buymeacoffee.com/pylin" target="_blank">
    <img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me a Coffee" height="36">
  </a>
  &nbsp;&nbsp;
  <a href="https://www.youtube.com/channel/UCKKhdFV0q8CV5vWUDfiDfTw" target="_blank">
    <img src="https://img.shields.io/badge/SUBSCRIBE%20ON%20YOUTUBE-FF0000?style=for-the-badge&logo=youtube&logoColor=white" alt="Subscribe on YouTube" height="36">
  </a>
</p>

[![YouTube](https://img.shields.io/badge/YouTube-Fusion_Automate-FF0000?logo=youtube&logoColor=white)](https://www.youtube.com/channel/UCKKhdFV0q8CV5vWUDfiDfTw)
[![Telegram](https://img.shields.io/badge/Telegram-%40fusionautomate-26A5E4?logo=telegram&logoColor=white)](https://t.me/fusionautomate)
[![Instagram](https://img.shields.io/badge/Instagram-%40fusion__automate-E4405F?logo=instagram&logoColor=white)](https://www.instagram.com/fusion_automate)
[![WhatsApp](https://img.shields.io/badge/WhatsApp-%2B91--9974477759-25D366?logo=whatsapp&logoColor=white)](https://wa.me/919974477759)
[![Buy Me a Coffee](https://img.shields.io/badge/Buy_Me_a_Coffee-support-FFDD00?logo=buymeacoffee&logoColor=black)](https://buymeacoffee.com/pylin)
</div>

# T-HMI Examples

This folder contains tracked example documentation and sketches for the LILYGO T-HMI ESP32-S3 touchscreen board. The examples cover display tests, LVGL touch interfaces, and LED control. The Modbus TCP project is retained only in some local working trees and is not available from a fresh repository clone.

## Projects

| Folder | What it demonstrates | Project status |
| --- | --- | --- |
| `Analogue_Meter_Example` | Two animated analog gauges using TFT_eSPI widgets | Arduino sketch; no project-local `platformio.ini` yet |
| `Colour_Test_Example` | Display colors, text, logo, and inversion | Arduino sketch; no project-local `platformio.ini` yet |
| `mbtcp_coil_control` *(not tracked)* | LVGL touchscreen control of Modbus TCP relay coils over Wi-Fi | Local working-tree project only; unavailable from a fresh clone |
| `neo_pixel_led_control` | LVGL touchscreen control of WS2812B/NeoPixel LEDs | Arduino sketch; no project-local `platformio.ini` yet |
| `t-hmi_led_control` | LVGL touchscreen interface to operate the board's LED | Arduino sketch; no project-local `platformio.ini` yet |
| `Bluetooth_MP3_Player` | 🛑 Investigation write-up: why a Bluetooth MP3 player is not possible on this board without added hardware (port intentionally halted) | Documentation only — no firmware |

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
