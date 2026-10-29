# Bluetooth MP3 Player for the T-HMI — investigated, intentionally not built

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
>    bare T-HMI cannot produce sound through *any* means.
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

## The research (done before halting — reusable if ever revisited)

### Proven architecture to port

The [T4-S3 player](../../../T4%20S3/Project/Bluetooth_MP3_Player/README.md)
already solves the "ESP32-S3 has no Bluetooth Classic" problem and its
firmware is the port template:

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

- `firmware-t4s3/src/audio/` (Mp3Player, PcmRing, Resampler),
  `src/Id3Parser.*`, `src/bt/BridgeLink.*` and `include/bridge_protocol.h`
  are hardware-agnostic and port verbatim — the only mechanical change is
  `SD.open(...)` → `SD_MMC.open(...)` (2 call sites).
- `firmware-bridge-esp32/` (A2DP radio dongle) is 100 % reusable unchanged;
  its UART pins are build flags.
- The UI (`src/ui/`) is LVGL 8.4 and ports with a re-layout: the T-HMI runs
  **landscape 320×240** — the same shape as the T4 V1.3 player screen.

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
| **Speaker edition** | ~$2 MAX98357A I2S amp + any small speaker; 4 wires: BCLK→IO15, LRC→IO16, DIN→IO17, power from Grove VDD3+GND. No second chip, no pairing. | Touch MP3 player that plays the SD card through the wired speaker. The `ENABLE_LOCAL_SPEAKER` code path already exists in the T4-S3 firmware. |
| **Bluetooth edition** | ~$4 classic-ESP32 devkit as the A2DP bridge dongle (must be classic ESP32, *not* S3/C3), wired to Grove2 as drawn above. | The full original experience: scan → tap → stream to any Bluetooth headphone/speaker. |

Both paths reuse the T4-S3 codebase plus the verified facts in the table
above; the port is mostly a `main.cpp` hardware-init swap.

## Where a *working* Bluetooth MP3 player lives in this repository

| Project | Why it works there |
|---|---|
| [T4 V1.3 Bluetooth MP3 Player](../../../T4%20V1.3/Project/Bluetooth_MP3_Player/README.md) | original ESP32 has Bluetooth Classic built in — streams standalone, hardware-tested |
| [T4-S3 Bluetooth MP3 Player](../../../T4%20S3/Project/Bluetooth_MP3_Player/README.md) | ESP32-S3 + the bridge-dongle architecture described here, build-verified |
