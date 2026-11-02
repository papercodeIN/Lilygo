<div align="center">

<img src="https://visitor-badge.laobi.icu/badge?page_id=papercodeIN.Lilygo" alt="Repository visitors" height="20" />

# LILYGO Projects

**Open-source firmware, examples, and build notes for LILYGO development boards**

[![Boards](https://img.shields.io/badge/hardware-LILYGO-2F4F7F)](https://lilygo.cc/)
[![Framework](https://img.shields.io/badge/framework-Arduino_/_PlatformIO-00979D?logo=arduino)](https://platformio.org/)
[![UI](https://img.shields.io/badge/UI-LVGL-32A3CE)](https://lvgl.io/)
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

---

## About

This repository collects firmware, examples, and project notes for LILYGO boards used in Fusion Automate builds: the **T-HMI** ESP32-S3 touchscreen, **T-Internet-COM** Ethernet board, and **T4** series (V1.3 and S3).

Project readiness varies. Some folders are complete PlatformIO projects; others are Arduino sketches that need board-specific libraries and configuration. Check each project README for requirements, wiring, tested status, and build instructions before flashing.

## Projects

### T-HMI — ESP32-S3 touchscreen ([product](https://lilygo.cc/products/t-hmi) · [official GitHub](https://github.com/Xinyuan-LilyGO/T-HMI))

See the [T-HMI board page](<T-HMI/README.md>) and [tracked examples](<T-HMI/Examples/README.md>) for setup notes and project status.

| Example | Description |
|---|---|
| [USB Stream Deck](<T-HMI/Examples/T-HMI USB Stream Deck/README.md>) | Touchscreen shortcut panel that sends USB HID media keys and keyboard shortcuts |
| [NeoPixel LED Control](<T-HMI/Examples/neo_pixel_led_control/README.md>) | Touchscreen RGB and brightness controls for a WS2812B/NeoPixel strip; read the wiring-safety notes before connecting hardware |
| [Touchscreen LED Control](<T-HMI/Examples/t-hmi_led_control/README.md>) | LVGL controls for the board's LED; Arduino sketch, not a standalone PlatformIO project |
| [Analogue Meter](<T-HMI/Examples/Analogue_Meter_Example/README.md>) | Animated TFT_eSPI gauges using generated demonstration values, not sensor readings |
| [Colour Test](<T-HMI/Examples/Colour_Test_Example/README.md>) | Display logo, color, and inversion test |
| [Bluetooth MP3 Player investigation](<T-HMI/Examples/Bluetooth_MP3_Player/README.md>) | Archived investigation; audio output requires external hardware and development was halted |

> The Modbus TCP example is not included in repository clones because its folder is excluded from version control.

### T4 V1.3 — ESP32 with ILI9341 display ([product](https://lilygo.cc/products/t4) · [official GitHub](https://github.com/Xinyuan-LilyGO/LilyGo_Txx))

| Project | Description |
|---|---|
| [ESP32 NAS](<T4 V1.3/Project/ESP32_NAS/README.md>) | microSD web file manager and Wi-Fi setup/status interface for the 2.4-inch display configuration |
| [Bluetooth MP3 Player](<T4 V1.3/Project/Bluetooth_MP3_Player/README.md>) | Hardware-tested MP3 player that streams from microSD to Bluetooth audio devices using A2DP |

### T4 S3 — ESP32-S3 with 2.41-inch AMOLED ([product](https://lilygo.cc/products/t4-s3) · [official GitHub](https://github.com/Xinyuan-LilyGO/LilyGo-AMOLED-Series))

| Project | Description |
|---|---|
| [Bluetooth MP3 Player — halted](<T4 S3/Project/Bluetooth_MP3_Player/README.md>) | Archived experiment, not in development. The ESP32-S3 cannot transmit Bluetooth Classic audio directly; Bluetooth requires a separate Classic-capable module. Wired audio requires external audio output hardware; a 3.5 mm jack is optional, not a board requirement. |

### Board references

| Board | Contents |
|---|---|
| [T-HMI](<T-HMI/README.md>) | Board resources, examples, video links, and configuration notes |
| [T-Internet-COM](<T-Internet-COM/README.md>) | Board overview and official resources |
| [T4 V1.3](<T4 V1.3/README.md>) | Board overview, repository projects, and video references |
| [T4 S3](<T4 S3/README.md>) | Board overview and project archive status |

## Video walkthroughs

Build and demo playlists on the [Fusion Automate YouTube channel](https://www.youtube.com/channel/UCKKhdFV0q8CV5vWUDfiDfTw):

- [LILYGO T-HMI](https://www.youtube.com/playlist?list=PLxrSjjYyzaaKcEFeebqFz8opz2ZnfCc98)
- [LILYGO T4 S3](https://www.youtube.com/playlist?list=PLxrSjjYyzaaLk5dXxcTvpG-8Y8nO3NJWK)
- [LILYGO T4 V1.3](https://www.youtube.com/playlist?list=PLxrSjjYyzaaJ7l1INiPHKAxmEzhema-9E)

## Getting started

1. Choose a board or project from the tables above.
2. Read its README for required hardware, dependencies, wiring, and build status.
3. Use Arduino IDE/CLI or PlatformIO as specified by that project; sketch-only examples may need additional setup.
4. Find boards and product details at [LILYGO](https://lilygo.cc/).
