<div align="center">

<img src="https://visitor-badge.laobi.icu/badge?page_id=papercodeIN.Lilygo.T-HMI" alt="T-HMI page visitors" height="20" />

# LILYGO T-HMI

**ESP32-S3 touchscreen board: setup notes, examples, and project status**

[Official GitHub](https://github.com/Xinyuan-LilyGO/T-HMI) · [Product page](https://lilygo.cc/products/t-hmi)

[![YouTube](https://img.shields.io/badge/YouTube-Fusion_Automate-FF0000?logo=youtube)](https://www.youtube.com/channel/UCKKhdFV0q8CV5vWUDfiDfTw)
[![Telegram](https://img.shields.io/badge/Telegram-%40fusionautomate-26A5E4?logo=telegram&logoColor=white)](https://t.me/fusionautomate)
[![Instagram](https://img.shields.io/badge/Instagram-%40fusion__automate-E4405F?logo=instagram&logoColor=white)](https://www.instagram.com/fusion_automate)

</div>

---

## Overview

This folder contains notes and examples for the LILYGO T-HMI ESP32-S3 touchscreen board. Project readiness and dependencies vary: some examples are Arduino sketches rather than complete PlatformIO projects. Check the individual README before building or wiring external hardware.

## Tested configuration reference

The following versions and Arduino IDE settings are recorded from the existing T-HMI setup. Library and board-core compatibility can vary; consult the project README if a specific example requires a different configuration.

| Component | Recorded version |
|---|---:|
| LVGL | 8.3.5 |
| lv_example | 8.1.0 |
| TFT_eWidget | 0.0.5 |
| TFT_eSPI | 2.5.43 |
| ESP32 Arduino core | 2.0.17 |

| Arduino IDE setting | Recorded value |
|---|---|
| Board | ESP32S3 Dev Module |
| USB CDC On Boot | Enabled |
| CPU frequency | 240 MHz (Wi-Fi) |
| Flash mode / size | QIO 80 MHz / 16 MB |
| PSRAM | OPI PSRAM |
| Partition scheme | Huge APP (3 MB No OTA / 1 MB SPIFFS) |
| USB mode | Hardware CDC and JTAG |

These are reference settings, not a universal configuration for every example or core version.

## Examples and projects

See the [examples index](<Examples/README.md>) for build status and setup guidance.

| Folder | Description | Readiness |
|---|---|---|
| [USB Stream Deck](<Examples/T-HMI USB Stream Deck/README.md>) | USB HID shortcut and media-key controller | PlatformIO project |
| [Analogue Meter](<Examples/Analogue_Meter_Example/README.md>) | Animated demonstration gauges using generated values | Arduino sketch; requires TFT_eSPI/TFT_eWidget configuration |
| [Colour Test](<Examples/Colour_Test_Example/README.md>) | Logo, color, and display-inversion test | Arduino sketch; requires T-HMI TFT_eSPI configuration |
| [Touchscreen LED Control](<Examples/t-hmi_led_control/README.md>) | LVGL controls for the board LED | Arduino sketch; not a standalone PlatformIO project |
| [NeoPixel LED Control](<Examples/neo_pixel_led_control/README.md>) | Touchscreen controls for external WS2812B LEDs | Arduino sketch; review wiring and power-safety notes |
| [Bluetooth MP3 Player investigation](<Examples/Bluetooth_MP3_Player/README.md>) | Hardware capability investigation; port intentionally halted | Documentation only |

The Modbus TCP project is currently excluded from version control and is not present in a fresh clone.

## Video guides

### Short demonstrations

| Video | Link |
|---|---|
| TFT demo | [Watch](https://youtube.com/shorts/9CcW0_vulEw) |
| Board unboxing | [Watch](https://youtube.com/shorts/BKtSJceH6yc) |
| Touchscreen GPIO control | [Watch](https://youtube.com/shorts/G9RggduKXN4) · [upstream example](https://github.com/Xinyuan-LilyGO/T-HMI/tree/master/examples/LVGL_Examples/T-HMI_Led_Control) |

### Long-form tutorials

| Tutorial | Video / code |
|---|---|
| Getting started with Arduino IDE | [Watch](https://youtu.be/lspoB5OtbuI) |
| Analogue Meter | [Watch](https://youtu.be/BZeGNBP6ork) · [repository example](<Examples/Analogue_Meter_Example/>) |
| Colour Test | [Watch](https://youtu.be/8henVYCI218) · [repository example](<Examples/Colour_Test_Example/>) |
| Touchscreen GPIO control | Tutorial forthcoming · [upstream example](https://github.com/Xinyuan-LilyGO/T-HMI/tree/master/examples/LVGL_Examples/T-HMI_Led_Control) |
| Modbus TCP relay control | Tutorial forthcoming; project is not tracked in this repository |
| NeoPixel control | Tutorial forthcoming · [repository example](<Examples/neo_pixel_led_control/>) |

## Support and contact

- [YouTube — Fusion Automate](https://www.youtube.com/channel/UCKKhdFV0q8CV5vWUDfiDfTw)
- [Telegram — @fusionautomate](https://t.me/fusionautomate)
- [WhatsApp — +91-9974477759](https://wa.me/919974477759)
- [Email](mailto:eng.innovativ@gmail.com)
