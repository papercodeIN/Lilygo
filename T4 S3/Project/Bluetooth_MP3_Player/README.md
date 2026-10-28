# Bluetooth MP3 Player — LilyGo T4-S3

> ### ⚠️ WORK IN PROGRESS — NOT YET TESTED ON HARDWARE
> Both firmwares **compile cleanly**, but they have **not yet been flashed or
> verified on real boards**. This project is in build mode — flash it and try
> it **at your own risk**. If something does not work, open an issue or check
> the [Troubleshooting](#troubleshooting) section below.

A touch-driven MP3 player for the LilyGo T4-S3 (2.41" AMOLED): browse the SD
card, **scan nearby Bluetooth headphones / speakers on screen, tap to
connect**, and control playback with a full transport UI (play/pause,
prev/next, seek bar, volume) — all by touch.

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

## Why there is a second chip

**The T4-S3's ESP32-S3 chip cannot talk to Bluetooth headphones on its own —
this is silicon, not software.**

| Capability | ESP32-S3 (T4-S3) | Needs for headphones |
|---|---|---|
| Bluetooth Classic (BR/EDR) | ❌ not present | **A2DP** — the protocol *every* BT headphone/speaker speaks |
| Bluetooth LE | ✅ | — |
| LE Audio (LC3 over CIS) | ❌ controller has no ISO channels; Espressif confirmed S3 will never support it | future headphones only |

So this project puts the entire player (UI, SD, MP3 decoding, battery,
volume, AVRCP) on the T4-S3 and adds a **$3–5 companion dongle** (any classic
ESP32 dev board) that acts purely as the Bluetooth radio. You scan and pick
the device on the touchscreen; the dongle is a dumb wireless cable. Audio is
standard A2DP/SBC, so **every Bluetooth headphone and speaker ever made
works**.

## What you need

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

## Build and flash from Visual Studio Code (step by step)

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

## Using the player (100% touch)

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

## Troubleshooting

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
- The project is **not hardware-tested yet** — first-boot tuning may be
  needed (scan timing, buffer sizes). Serial logs at 115200 will pinpoint
  issues.

## Optional: local speaker output

No dongle, no Bluetooth — wire a **MAX98357A** I2S amp to the header and play
through a small speaker instead. Set `ENABLE_LOCAL_SPEAKER 1` in
`firmware-t4s3/include/config.h`, wire BCLK←GPIO38, LRC←GPIO39, DIN←GPIO40,
Vin←VDD3V3. An "Onboard speaker" entry appears at the top of the device list.

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
