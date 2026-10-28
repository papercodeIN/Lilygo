<div align="center">

<img src="https://visitor-badge.laobi.icu/badge?page_id=papercodeIN.Lilygo" alt="visitors" height="20" />

# LILYGO Projects

**Open-source firmware, examples and build guides for LILYGO development boards**

[![Boards](https://img.shields.io/badge/hardware-LILYGO-2F4F7F)](https://lilygo.cc/)
[![Framework](https://img.shields.io/badge/framework-Arduino_/_PlatformIO-00979D?logo=arduino)](https://platformio.org/)
[![GUI](https://img.shields.io/badge/UI-LVGL-32A3CE)](https://lvgl.io/)
[![YouTube](https://img.shields.io/badge/YouTube-Fusion_Automate-FF0000?logo=youtube)](https://www.youtube.com/channel/UCKKhdFV0q8CV5vWUDfiDfTw)

</div>

---

## About

Firmware, working examples and full project write-ups for the LILYGO boards used in Fusion Automate builds — the **T-HMI** ESP32-S3 HMI display, the **T-Internet-COM** Ethernet board, and the **T4** series (S3 and V1.3).

Every example and project folder documents its hardware setup, wiring and build steps so you can clone, open and flash without guesswork.

## Projects

### T-HMI — ESP32-S3 4-inch HMI display ([product](https://lilygo.cc/products/t-hmi) · [official GitHub](https://github.com/Xinyuan-LilyGO/T-HMI))

LVGL touchscreen examples — each folder is self-contained with wiring notes and build instructions ([all examples](<T-HMI/Examples/README.md>)):

| Example | Description |
|---|---|
| [USB Stream Deck](<T-HMI/Examples/T-HMI USB Stream Deck/README.md>) | Touch stream deck that sends HID keystrokes/macros to a PC over USB |
| [Modbus TCP Coil Control](<T-HMI/Examples/mbtcp_coil_control/README.md>) | Drive Modbus TCP coils from the touchscreen — industrial control demo |
| [NeoPixel LED Control](<T-HMI/Examples/neo_pixel_led_control/README.md>) | WS2812B strip control with on-screen RGB and brightness sliders (read the wiring-safety notes) |
| [Touchscreen LED Control](<T-HMI/Examples/t-hmi_led_control/README.md>) | On-board LED control from the LVGL UI |
| [Analogue Meter](<T-HMI/Examples/Analogue_Meter_Example/README.md>) | LVGL analogue-style gauge widget demo |
| [Colour Test](<T-HMI/Examples/Colour_Test_Example/README.md>) | Panel uniformity and touch test pattern |

### T4 V1.3 — ESP32 with 2.2-inch LCD

| Project | Description |
|---|---|
| [ESP32 NAS](<T4 V1.3/Project/ESP32_NAS/README.md>) | Network-attached storage with LCD status screen and browser file manager |
| [Digital Clock](<T4 V1.3/Project/Digital_Clock/Digital_Clock.ino>) | Standalone clock sketch for the T4 display |
| [Bluetooth MP3 Player](<T4 V1.3/Project/Bluetooth_MP3_Player/README.md>) | Standalone Bluetooth music player - plays MP3s from microSD through any Bluetooth speaker/headphones (A2DP), with on-screen device picker, volume and battery-ready design. Hardware-tested |

### T4 S3 — ESP32-S3 with 2.41-inch AMOLED ([product](https://lilygo.cc/products/t4-s3) · [official GitHub](https://github.com/Xinyuan-LilyGO/LilyGo-AMOLED-Series))

| Project | Description |
|---|---|
| [Bluetooth MP3 Player](<T4 S3/Project/Bluetooth_MP3_Player/README.md>) | Touch-controlled MP3 player — scans and streams to any Bluetooth headphone/speaker through a tiny ESP32 A2DP bridge dongle. ⚠️ Work in progress — not yet hardware-tested |

### Board references

| Board | Contents |
|---|---|
| [T-Internet-COM](<T-Internet-COM/README.md>) | ESP32 board with wired Ethernet — board overview and resources |
| [T4 S3](<T4 S3/README.md>) | ESP32-S3 development board — board overview and resources |

## Video walkthroughs

Build and demo playlists on the [Fusion Automate YouTube channel](https://www.youtube.com/channel/UCKKhdFV0q8CV5vWUDfiDfTw):

- [LILYGO T-HMI](https://www.youtube.com/playlist?list=PLxrSjjYyzaaKcEFeebqFz8opz2ZnfCc98)
- [LILYGO T4 S3](https://www.youtube.com/playlist?list=PLxrSjjYyzaaLk5dXxcTvpG-8Y8nO3NJWK)
- [LILYGO T4 V1.3](https://www.youtube.com/playlist?list=PLxrSjjYyzaaJ7l1INiPHKAxmEzhema-9E)

## Getting started

1. Pick an example or project from the tables above and open its folder.
2. Follow that folder's README — hardware requirements, wiring and build/flash steps (VS Code + PlatformIO or Arduino IDE).
3. Boards are available from [LILYGO](https://lilygo.cc/).

## Support

If these projects help you, consider supporting the channel:

<a href="https://buymeacoffee.com/pylin" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me a Coffee" height="36"></a>
&nbsp;&nbsp;
<a href="https://www.youtube.com/channel/UCKKhdFV0q8CV5vWUDfiDfTw" target="_blank"><img src="https://img.shields.io/badge/SUBSCRIBE%20ON%20YOUTUBE-FF0000?style=for-the-badge&logo=youtube&logoColor=white" alt="Subscribe on YouTube" height="36"></a>

## Contact

| Method | Details |
|---|---|
| Call / WhatsApp | [+91-9974477759](https://wa.me/919974477759) |
| Telegram | [@fusionautomate](https://t.me/fusionautomate) |
| Email | [eng.innovativ@gmail.com](mailto:eng.innovativ@gmail.com) |
