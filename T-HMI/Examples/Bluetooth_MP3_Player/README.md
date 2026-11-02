<div align="center">

<img src="https://visitor-badge.laobi.icu/badge?page_id=papercodeIN.Lilygo.T-HMI.Bluetooth_MP3_Player" alt="Investigation visitors" height="20" />

# Bluetooth MP3 Player for the LILYGO T-HMI

[![YouTube](https://img.shields.io/badge/YouTube-Fusion_Automate-FF0000?logo=youtube)](https://www.youtube.com/channel/UCKKhdFV0q8CV5vWUDfiDfTw)
[![Telegram](https://img.shields.io/badge/Telegram-%40fusionautomate-26A5E4?logo=telegram&logoColor=white)](https://t.me/fusionautomate)

</div>

> **Archive status: investigated and intentionally not built.**

> ### 🛑 STATUS — READ FIRST
> **This folder intentionally contains documentation only — no firmware.**
> The port of the Bluetooth MP3 player to the LILYGO T-HMI was fully
> researched (2026-10-29) and then **halted by owner decision**, because the
> T-HMI hardware cannot deliver what the project requires:
>
> 1. The T-HMI's ESP32-S3 chip has **no Bluetooth Classic**, so it cannot
>    stream to any Bluetooth headphone or speaker on its own — ever.
> 2. The board has **no audio hardware at all** (no DAC, no amplifier, no
>    speaker, no headphone jack — verified in the official schematic), so a
>    bare T-HMI cannot produce wired sound without external audio hardware.
>
> No firmware, library, or setting can change either fact. This README
> preserves the complete investigation so a future attempt starts from zero
> research instead of repeating it.

---

## Why the T-HMI cannot play Bluetooth audio (verified)

| Capability | T-HMI (ESP32-S3) | What Bluetooth headphones need |
|---|---|---|
| Bluetooth Classic (BR/EDR) | ❌ not present in the silicon | **A2DP** — the profile *every* BT headphone/speaker speaks |
| Bluetooth LE | ✅ | — (useless for A2DP headphones) |
| LE Audio (LC3 over CIS) | ❌ controller has no ISO channels; Espressif has stated the S3 will never support it | future LE-Audio headphones only |

Evidence (same verification as the
[T4-S3 player](../../../T4%20S3/Project/Bluetooth_MP3_Player/README.md), which
hit the identical wall):

- Espressif's official chip page lists the ESP32-S3 radio as
  "Bluetooth 5 (LE)" — Bluetooth Classic is never mentioned because it does
  not exist on this chip.
- ESP-IDF's `soc/esp32s3/include/soc/soc_caps.h` defines
  `SOC_BLE_SUPPORTED` but has **no** `SOC_BT_CLASSIC_SUPPORTED`. The original
  ESP32's header *does* define it (`SOC_BT_CLASSIC_SUPPORTED (1)`) — exactly
  the hardware A2DP needs.
- The T-HMI official examples and pinout poster confirm the MCU is an
  **ESP32-S3** (16 MB flash, 8 MB PSRAM).

**What a scan on the T-HMI's own radio would find:** a BLE scan only. Real
headphones/speakers advertise on Bluetooth Classic, so they can never appear
in an on-screen device list without a second, classic-Bluetooth radio.

## Why it cannot play *any* audio without added hardware

The official T-HMI schematic (`schematic/Schematic.pdf` in
[Xinyuan-LilyGO/T-HMI](https://github.com/Xinyuan-LilyGO/T-HMI)) was checked
end to end. Audio-related findings:

| Part | Present? | Notes |
|---|---|---|
| Audio DAC / codec | ❌ none | — |
| Speaker / amplifier | ❌ none | the only LED-driver chip (AW9364) is for the backlight |
| Headphone jack | ❌ none | — |
| TP4056 battery charger | ✅ | its `CHRG` status pin drives the charge LED only — **not routed to any GPIO**, so charging state is not readable in software |

Conclusion: a bare T-HMI is a silent device. **Any** sound requires at least
a small external amp module; Bluetooth streaming additionally requires a
second chip with Bluetooth Classic.

---

## The research (record retained for reference)

### Previously explored architecture (not hardware-validated)

The archived [T4-S3 player](../../../T4%20S3/Project/Bluetooth_MP3_Player/README.md)
explored a Bluetooth Classic bridge dongle to work around the ESP32-S3's
Bluetooth limitation, but its development was halted before end-to-end audio
was tested on hardware. It is not a verified working solution or maintained
port template:

```
┌────────────────────────────┐        UART 2 Mbit         ┌──────────────────────┐
│  T-HMI (this board)        │  T-HMI IO17 ──▶ RX2(16)    │  ESP32 bridge dongle │
│                            │  T-HMI IO18 ◀── TX2(17)    │  (any classic ESP32) │
│  LVGL UI  ← XPT2046 touch  │     (Grove2)               │                      │
│  SD card → MP3 → PCM ──────│───────────────────────────▶│  PCM → SBC ──▶ A2DP  │
│  Battery % via ADC         │                            │        │             │
└────────────────────────────┘                            └────────┼─────────────┘
                                                                   ▼
                                                   🎧 any Bluetooth headphone/speaker
```

- The T4-S3 audio, ID3, and bridge-link modules were identified as possible
  porting candidates; no T-HMI port was made or verified, and compatibility
  should not be assumed from this investigation alone.
- The `firmware-bridge-esp32/` A2DP dongle firmware build was verified, but
  reuse with a T-HMI and end-to-end audio were not tested on hardware.
- The T4-S3 UI uses LVGL 8.4; a T-HMI port would need a layout change for its
  **landscape 320×240** display. No such port was implemented.

### T-HMI hardware facts verified against official sources

All from the official repo's examples, pinout poster and schematic
(`Xinyuan-LilyGO/T-HMI`), cross-checked against this repository's own
T-HMI examples:

| Subsystem | Verified facts |
|---|---|
| Display | ST7789V, 240×320, **8-bit Intel 8080 parallel** (data D0–D7 on GPIO 48/47/39/40/41/42/45/46, PCLK 8, CS 6, DC 7, no reset line). Drive via ESP-IDF `esp_lcd` i80 bus + `esp_lcd_new_panel_st7789` at 10 MHz, `swap_color_bytes=1` (so LVGL uses `LV_COLOR_16_SWAP 0`). Landscape = `mirror(false,true)` + `swap_xy(true)`, LVGL `hor_res=320, ver_res=240`. Backlight GPIO 38 (AW9364 driver: ON/OFF only — no reliable brightness PWM). |
| Touch | XPT2046 resistive on its own SPI (SCLK 1, MISO 4, MOSI 3, CS 2, IRQ 9). Proven mapping: `setCal(285, 1788, 311, 1877, 240, 320)` + `setRotation(3)` yields coordinates in the same 320×240 landscape space LVGL uses. The bundled library (`liangyingy/arduino_xpt2046_library`) has a stray `Serial.println("Pressed!")` per poll — vendor a cleaned copy. |
| SD card | mounts with **SD_MMC 1-bit mode**: `SD_MMC.setPins(CLK=12, CMD=11, D0=13); SD_MMC.begin("/sdcard", true)` (official `examples/sd`). Faster than SPI and leaves the touch SPI bus alone. |
| Battery | voltage on **GPIO5** via ADC (÷2 resistor divider on the board — multiply the ADC voltage by 2, as in official `examples/adc`). ⚠️ The official pinout poster prints "BAT_ADC IO03" — that is a typo (IO3 is the touch MOSI); every official example's `pins.h` says 5. No charging-status pin exists (see schematic table above), so % is voltage-curve based and the charge icon cannot be driven. |
| Power | **GPIO14 = PWR_ON** must be driven HIGH at boot to latch power (and LOW = soft power-off); **GPIO10 = PWR_EN** must be HIGH when running on battery (gates the peripheral rail). External 5 V DC seat needs a magnet on the reed switch or the latch set. Reed switch on **GPIO21**; BOOT button **GPIO0**. |
| Free pins for the bridge/speaker | **Grove2 = GND, VDD3, IO17, IO18** (use IO17 TX, IO18 RX for the bridge UART); Grove1 = GND, VDD3, IO15, IO16; Grove3 = UART0 (GND, VDD3, IO43, IO44). GPIO21 is the reed switch — not free. |
| LVGL config | 8.4 works; memory allocator `ps_malloc`/`ps_realloc` (factory example pattern), tick via `millis()`, fonts 12–28 are enough for a 320×240 layout. |

### Original product context

- Product: <https://lilygo.cc/products/t-hmi> ·
  Official repo: <https://github.com/Xinyuan-LilyGO/T-HMI>
- Requested UX (for the record): boot → on-screen scan of nearby Bluetooth
  headphones/speakers → tap to connect → play, with next/prev and
  volume ± buttons on screen, track name top-centre, battery and Bluetooth
  status in the top bar.

---

## If the project is ever revived, the two workable paths

| Path | Extra hardware | Result |
|---|---|---|
| **Speaker edition** | External I2S DAC/amplifier and speaker, wired to the T-HMI. | A possible wired-audio approach; not implemented or tested. |
| **Bluetooth edition** | A separate Bluetooth Classic-capable A2DP module/adapter, connected to the T-HMI. | A possible Bluetooth-audio approach; not implemented or tested. |

These are theoretical approaches only. The archived T4-S3 code is not a
maintained or hardware-validated port for the T-HMI, and further development
has been halted.

## Related Bluetooth MP3 player projects in this repository

| Project | Status |
|---|---|
| [T4 V1.3 Bluetooth MP3 Player](../../../T4%20V1.3/Project/Bluetooth_MP3_Player/README.md) | original ESP32 has Bluetooth Classic built in — streams standalone, hardware-tested |
| [T4-S3 Bluetooth MP3 Player](../../../T4%20S3/Project/Bluetooth_MP3_Player/README.md) | Archived ESP32-S3 + bridge-dongle experiment; development halted and end-to-end audio not hardware-tested |
