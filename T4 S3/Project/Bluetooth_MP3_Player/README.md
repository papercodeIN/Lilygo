<div align="center">

<img src="https://visitor-badge.laobi.icu/badge?page_id=papercodeIN.Lilygo.T4_S3.Bluetooth_MP3_Player" alt="Project visitors" height="20" />

# Bluetooth MP3 Player — LILYGO T4-S3

[![YouTube](https://img.shields.io/badge/YouTube-Fusion_Automate-FF0000?logo=youtube)](https://www.youtube.com/channel/UCKKhdFV0q8CV5vWUDfiDfTw)
[![Telegram](https://img.shields.io/badge/Telegram-%40fusionautomate-26A5E4?logo=telegram&logoColor=white)](https://t.me/fusionautomate)

</div>

> **Project status: development halted.**

> ### 🛑 STATUS — NOT IN DEVELOPMENT
> **Development has been stopped; this project will not be progressed further.**
> The T4-S3's ESP32-S3 cannot transmit audio directly over Bluetooth because
> it lacks Bluetooth Classic, and the board has no built-in audio output or
> 3.5 mm headphone jack.
>
> Audio would require additional hardware: wired audio needs an external
> audio DAC/amplifier and a suitable output device such as a speaker or
> headphone jack; Bluetooth audio needs a separate Bluetooth Classic
> module/adapter. The bridge-dongle design below is only a proposed
> workaround, not a completed or supported solution.
>
> This README and the source are retained for historical reference. Firmware
> builds and the T4-S3 UI boot were verified, but end-to-end audio was not
> tested on hardware. This is not a work-in-progress project, and no further
> development or audio testing is planned.

The material below records the original touch-driven MP3 player concept and
its proposed external-hardware workarounds for archival reference only.

```
┌─────────────────────────────┐        UART 2 Mbit        ┌──────────────────────┐
│  T4-S3 (this project)       │  T4-S3 GPIO21 ──▶ RX(16)  │  ESP32 bridge dongle │
│                             │  T4-S3 GPIO47 ◀── TX(17)  │  (any ESP32 devkit)  │
│  AMOLED UI  ← touch         │                           │                      │
│  SD card  → MP3 → PCM ──────│──────────────────────────▶│  PCM → SBC ──▶ A2DP  │
│  Battery % / charging       │                           │        │             │
└─────────────────────────────┘                           └────────┼─────────────┘
                                                                    ▼
                                                    🎧 any Bluetooth headphone/speaker
```

---

## Hardware limitation: why the proposed Bluetooth design used a second chip

**The T4-S3's ESP32-S3 chip cannot talk to Bluetooth headphones on its own —
this is silicon, not software. No firmware change, library, or setting can
fix it.** Verified against Espressif's own documentation and source code:

| Capability | ESP32-S3 (T4-S3) | Needs for headphones |
|---|---|---|
| Bluetooth Classic (BR/EDR) | ❌ not present | **A2DP** — the protocol *every* BT headphone/speaker speaks |
| Bluetooth LE | ✅ | — |
| LE Audio (LC3 over CIS) | ❌ controller has no ISO channels; Espressif confirmed S3 will never support it | future headphones only |

**Proof from Espressif (checked 2026-10):**

| # | Evidence | Source | What it proves |
|---|---|---|---|
| 1 | Official chip page | [espressif.com/en/products/socs/esp32-s3](https://www.espressif.com/en/products/socs/esp32-s3) | Radio is "**Bluetooth 5 (LE)**" only — Bluetooth Classic (BR/EDR) is never mentioned, because it does not exist on this chip |
| 2 | ESP-IDF capability header for the S3 | `components/soc/esp32s3/include/soc/soc_caps.h` | Only BLE macros defined (`SOC_BLE_SUPPORTED`, `SOC_BLE_50_SUPPORTED`…). **No `SOC_BT_CLASSIC_SUPPORTED`, no `SOC_BLE_ISO_SUPPORTED`, no `SOC_BLE_AUDIO_SUPPORTED`** |
| 3 | Same header for the original ESP32 | `components/soc/esp32/include/soc/soc_caps.h` | **Has `SOC_BT_CLASSIC_SUPPORTED (1)`** — exactly the hardware A2DP needs |
| 4 | LE Audio support matrix | ESP-IDF "BLE overview" docs; [esp-idf issue #12277](https://github.com/espressif/esp-idf/issues/12277) | `ESP-BLE-ISO` / `ESP-BLE-AUDIO`: only ESP32-H4/S31 = "Y"; **ESP32-S3 = "–"** — Espressif staff confirmed the S3 will never get LE Audio |

**Why a device scan finds nothing without the dongle:**

1. The on-screen device list is filled by the **bridge dongle** doing a
   Bluetooth **Classic** scan and forwarding results over UART. With no
   dongle attached, the UI shows *"Bridge dongle not connected"* and lists no
   devices — designed behavior, not a failure.
2. A scan made with the T4-S3's own radio is a **BLE** scan. Headphones and
   speakers advertise on **Bluetooth Classic** (A2DP), a protocol the S3's
   radio does not have — so they can never appear in the list.
3. LE Audio (the BLE-based successor) is equally impossible: the S3's
   controller has **no ISO channels**, which LE Audio requires.

**Hardware workarounds considered (not developed into a supported product):**

- Bluetooth audio would require a separate Bluetooth Classic-capable module
  or adapter; the archived design describes using a classic-ESP32 dev board
  as an A2DP bridge over UART.
- Wired audio would require external audio output hardware (a DAC/amplifier)
  and a suitable output such as a speaker or headphone jack. The board has no
  built-in audio output.
- A different board with Bluetooth Classic built in, such as the original
  ESP32 used by the TTGO T4 V1.3, can transmit without a separate bridge.

These are possible hardware approaches, not tested or supported solutions
for this halted T4-S3 project.

## Archived hardware notes (not a current build recommendation)

| Item | Notes |
|---|---|
| LilyGo T4-S3 | the player |
| ESP32 dev board (e.g. ESP32-DevKitC, ESP32 WROOM-32, ~$4) | the bridge dongle — must be **classic ESP32**, not S3/C3 |
| 4 female-female Dupont wires | UART link + ground |
| USB-C cable (T4-S3) + micro-USB cable (dongle) | for flashing |
| PC with VS Code | free — see steps below |
| microSD card, FAT32 | MP3 files |
| 3.7 V Li-Po battery (JST-GH 1.25 mm plug) | optional, e.g. 1000–2000 mAh fits behind the board |

---

## Archived build notes (historical reference only)

### Historical setup steps (provided for reference only)

The following steps document the original experiment. They are not maintained
or recommended as a working T4-S3 audio player setup.

### Step 1 — Install Visual Studio Code

1. Download VS Code from <https://code.visualstudio.com> and install it
   (default options are fine).

### Step 2 — Install the PlatformIO extension

1. Open VS Code.
2. Click the **Extensions** icon in the left sidebar (or press
   `Ctrl+Shift+X`).
3. Search for **PlatformIO IDE**, click **Install**, and wait until it says
   installed. The first activation downloads PlatformIO Core in the
   background — this can take a few minutes; let it finish.
4. A small **ant head** (PlatformIO) icon appears in the left sidebar when
   ready.

### Step 3 — Get this project onto your PC

Either clone with git:

```sh
git clone https://github.com/papercodeIN/Lilygo.git
```

…or on the GitHub page press the green **Code** button → **Download ZIP**,
then extract it. The player lives at
`Lilygo/T4 S3/Project/Bluetooth_MP3_Player` and contains two firmware
folders:

| Folder | Board | What it is |
|---|---|---|
| `firmware-t4s3` | LilyGo T4-S3 | the player itself (UI, SD, decoding, battery) |
| `firmware-bridge-esp32` | any classic-ESP32 devkit | the Bluetooth radio dongle |

### Step 4 — Flash the bridge dongle first

1. In VS Code: **File → Open Folder…** → select
   `Bluetooth_MP3_Player/firmware-bridge-esp32`.
2. Connect the ESP32 devkit with a micro-USB cable.
3. Click the **PlatformIO (ant) icon** → *Project Tasks* → `bridge` →
   **General → Upload** (or simply click the **→** Upload button in the
   bottom toolbar).
4. The first build downloads the ESP32 toolchain — several minutes, one time
   only. When the bar shows `SUCCESS`, the dongle is flashed.
5. If more than one COM port exists, pick the new one that appeared when you
   plugged the board in.

### Step 5 — Flash the T4-S3

1. **File → Open Folder…** → select
   `Bluetooth_MP3_Player/firmware-t4s3`.
2. Connect the T4-S3 with a USB-C cable.
3. Click **Upload** the same way. If the upload cannot connect, **hold the
   BOOT button on the back while plugging in the USB cable**, release, then
   click Upload again — that forces the ESP32-S3 into download mode.
4. Optional: open the **Serial Monitor** (plug icon in the bottom toolbar,
   baud `115200`) to watch the boot log.

### Step 6 — Wire the two boards together

| T4-S3 30-pin header | ESP32 devkit | Function |
|---|---|---|
| **GPIO21** | **GPIO16** | T4-S3 TX → bridge RX |
| **GPIO47** | **GPIO17** | T4-S3 RX ← bridge TX |
| **GND** | **GND** | common ground |
| **VDD3V3** | **3V3** | power the dongle from the player (see note) |

> **Powering the bridge:** two options.
> 1. **Battery/portable use** — feed the dongle's **3V3 pin** from the T4-S3's
>    **VDD3V3** header pin (bypasses the dongle's regulator). ⚠️ **Disconnect
>    that 3V3 wire whenever you power or flash the dongle over its own USB**,
>    otherwise the two supplies fight.
> 2. **Desk use** — power the dongle from its own USB cable; then only 3 wires
>    (TX, RX, GND) are needed.
>
> Other free header pins (GPIO38/39/40/41/42/48) work too — change
> `BRIDGE_UART_TX_PIN` / `BRIDGE_UART_RX_PIN` in
> `firmware-t4s3/include/config.h`. Keep wires short (<20 cm).

### Step 7 — Prepare the microSD card

1. Format the card **FAT32** (cards ≤32 GB are the most reliable).
2. Copy `.mp3` files into the root and/or one level of folders (up to 500
   tracks). Files starting with `.` or `._` are skipped.
3. Insert the card into the T4-S3 before powering on. Track names come from
   ID3v2 tags (title/artist) with the filename as fallback.
4. Any sample rate (8–48 kHz, mono or stereo) works — audio is converted to
   44.1 kHz stereo for the Bluetooth link.

### Step 8 — (Optional) attach a battery

- Connector: **JST-GH 1.25 mm**, 3.7 V single-cell Li-Po — a 1000–2000 mAh
  cell fits behind the board.
- **Double-check the wire polarity** against the marking on the board before
  plugging the connector in.
- The firmware charges gently (4.208 V termination, 704 mA) and shows the
  battery percentage top-right; the charger LED is disabled to save power.

### Step 9 — Power up

Connect the T4-S3 to USB (or battery) → splash screen → Bluetooth device
list. The dongle can be powered from the T4-S3 or its own USB — once both
are on, you never need to touch the dongle again.

---

## Original intended interface (not validated end to end)

1. **Boot** → splash → **Bluetooth device screen**. It never reconnects to
   the last device: every boot shows a **fresh scan** of audio devices
   nearby.
2. **Scroll the list**, **tap a device** to connect (the row highlights while
   connecting). Put headphones in pairing mode so they appear in the scan.
3. Connected → the **player screen** opens and playback starts:
   - track title (marquee) + artist + track counter at the top
   - **drag the progress bar to seek**, elapsed/total time on both ends
   - big **play/pause**, **previous** (restarts the track if >3 s in, else
     jumps back), **next** — auto-advance at end of album
   - **volume slider** (controls the headphones directly via AVRCP absolute
     volume; your headphone's own buttons also work and stay in sync)
   - **◀ back button** disconnects and returns to the device list
4. **Battery widget (top right)** on every screen: percentage pill that turns
   amber/red when low, ⚡ symbol while charging. Below 10% you get a warning;
   at 0% the player hibernates to protect the cell (BOOT button or charger
   wakes it).
5. Screen **dims after 15 s and sleeps after 60 s**; any touch wakes it.
   Playback continues in both states.

Headphone-side buttons (play/pause/next/prev/vol on the headset) are
forwarded to the player via AVRCP and control the T4-S3 UI.

## Archived troubleshooting notes (not maintained)

| Symptom | Fix |
|---|---|
| "Bridge dongle not connected" banner | Check TX↔RX are crossed (21→16, 47→17), GND common, dongle powered |
| Device list empty | Headphones must be in **pairing mode**; some sinks only advertise when disconnected from their previous host |
| "Connecting…" never completes | Device already grabbed by your phone — disconnect it there first, then tap again |
| Audio stutter | Shorten the UART wires; keep Wi-Fi off (it is off by design); confirm 2 Mbaud link (defaults are correct) |
| No sound, connected | Volume slider at 0, or headphones' own volume — raise either |
| SD card not detected | FAT32 format, ≤32 GB cards are the most reliable; reseat the card |
| Wrong battery % | Voltage-based estimate — give the gauge a minute after boot; keep the EMA in mind after plugging/unplugging |
| Upload fails on T4-S3 | Hold BOOT while plugging USB-C, then Upload again |

## Limitations (honest list)

- Bluetooth **Classic A2DP only** — this covers all normal
  headphones/speakers; LE-Audio-only future devices are out of scope
  (impossible on this chip).
- SBC codec (A2DP default) — good quality, ~200 ms latency; fine for music.
- Progress/duration for VBR files without a Xing header is estimated from
  bitrate (usually accurate to a few seconds).
- The proposed audio path was **not tested on hardware**. Development has
  been halted; this repository does not provide a maintained or supported
  implementation.

## Archived local-speaker experiment notes

These notes describe an unverified historical experiment, not a supported
configuration. No Bluetooth dongle is needed, but external audio hardware is:
wire a **MAX98357A** I2S amplifier and a small speaker to the header. Set
`ENABLE_LOCAL_SPEAKER 1` in `firmware-t4s3/include/config.h`, then wire
BCLK←GPIO38, LRC←GPIO39, DIN←GPIO40, and Vin←VDD3V3. An "Onboard speaker"
entry was intended to appear at the top of the device list.

## Repository layout

```
firmware-t4s3/            main firmware (PlatformIO)
  include/config.h        all tunables (pins, baud, timeouts, charge profile)
  include/lv_conf.h       LVGL config (fonts 12-48, PSRAM allocator) — copied to
                          the LVGL build by scripts/copy_lvconf.py pre-hook
  include/bridge_protocol.h   UART frame protocol (shared with the bridge)
  lib/LilyGo-AMOLED-Series/   vendored board library (examples pruned so
                          Windows' 260-char path limit can't break the build)
  src/main.cpp            board bring-up (portrait rotation, LVGL init)
  src/App.*               state machine, SD scan, event glue
  src/ui/                 LVGL screens (devices, player, battery widget, theme)
  src/audio/              Helix MP3 decode → resample → UART/I2S pump
  src/bt/BridgeLink.*     UART protocol client
  src/power/              SY6970 battery gauge + charging profile
firmware-bridge-esp32/    companion dongle firmware (A2DP source)
docs/T4-S3-schematic.pdf  official board schematic (header pinout reference)
```

## Credits

- [LilyGo-AMOLED-Series](https://github.com/Xinyuan-LilyGO/LilyGo-AMOLED-Series) — board library
- [ESP32-A2DP](https://github.com/pschatzmann/ESP32-A2DP) and
  [arduino-libhelix](https://github.com/pschatzmann/arduino-libhelix) by Phil Schatzmann
- [LVGL](https://lvgl.io) 8.4

---

A [Fusion Automate](https://www.youtube.com/channel/UCKKhdFV0q8CV5vWUDfiDfTw)
project — build and demo videos on YouTube.
