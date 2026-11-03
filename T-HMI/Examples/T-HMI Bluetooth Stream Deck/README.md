<div align="center">

<img src="https://visitor-badge.laobi.icu/badge?page_id=papercodeIN.Lilygo.T-HMI.Bluetooth_Stream_Deck" alt="Example visitors" height="20" />

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

# T-HMI Bluetooth Stream Deck

A wireless Bluetooth Stream Deck for the LilyGo T-HMI (ESP32-S3). The deck
pairs with your Windows, Linux, or macOS computer over Bluetooth LE and shows
up as a keyboard. The 320x240 touchscreen shows multiple swipeable pages of
buttons, and each tap sends a media key or keyboard shortcut to the connected
computer — no USB cable needed after flashing.

This is the wireless sibling of the [T-HMI USB Stream Deck](../T-HMI%20USB%20Stream%20Deck):
same config-driven page table and theme, but the buttons go over Bluetooth and
the small screen is split into swipeable pages.

> **Hardware status: tested and working.** Verified on a physical LilyGo
> T-HMI display: the swipeable multi-page UI renders correctly, and the deck
> pairs with Windows over Bluetooth LE and sends its configured actions as a
> HID keyboard.

## What You Need

- LilyGo T-HMI board
- A computer with Bluetooth (Windows 10/11, Linux, or macOS)
- USB-C cable (for flashing only; the deck runs on USB power or a phone charger)
- PlatformIO Core or the PlatformIO IDE extension for VS Code

Tested with:

- PlatformIO `espressif32@6.9.0`
- LVGL `8.3.11`
- ESP32-BLE-Keyboard `0.3.2`
- LilyGo T-HMI

## Features

- Bluetooth LE HID keyboard — pairs with any PC, no companion app or driver
- Multiple pages with 3x2 button grids (6 buttons per page)
- Swipe left/right to change pages, or tap the header arrows
- Page dots under the title show which page you are on, tinted with each
  page's accent color
- Per-page accent colors on icons, pressed borders, and page dots
- Dark tech theme with a faint static grid behind the sliding pages
- Status bar with live Bluetooth connection state and last-action feedback
- Media keys, keyboard shortcuts, and timed sequences (e.g. open Windows
  Terminal via Win+R)
- Config-driven pages and button actions
- Backlight idle dimming after 30 seconds
- Branded splash screen with the pairing name
- Compile-time and boot-time configuration validation

## Included Pages

| Page | Buttons |
| --- | --- |
| Media | Play/Pause, Next, Prev, Vol +, Vol -, Mute |
| Desktop | Start (Win), Alt+Tab, Snip (Win+Shift+S), Desktop (Win+D), Copy, Paste |
| Meetings | Mic, Cam, Hand, React, Leave, Emoji |
| OBS | Scene 1, Scene 2, Scene 3, Mic, Stream, Replay |
| System | Terminal (Win+R → `wt`), Esc, Ctrl+Tab, Undo, Redo, Lock (Win+L) |

The OBS buttons send `F13` through `F18`. Bind those keys inside OBS under
**Settings > Hotkeys** so they do not conflict with normal keyboard shortcuts.

The meeting buttons use common Microsoft Teams / Zoom / Google Meet shortcuts,
but these can vary by app and version. Remap them in `include/config.h` if
your app uses different shortcuts.

The Terminal button opens the Windows Run dialog, types `wt`, and presses
Enter (Windows Terminal). Remap or remove it for Linux/macOS use.

## Quick Start

1. Flash the deck (see below).
2. Wait for the splash screen. It shows the Bluetooth name: **T-HMI Deck**.
3. On the computer, open Bluetooth settings and pair with **T-HMI Deck**.
   It appears as a keyboard; pairing is "Just Works" (no PIN).
4. When pairing completes, the status bar shows **BT: Connected** and the
   action line shows **Ready**.
5. Tap any button to send its action. Swipe left/right (or tap the arrows)
   to change pages.

The deck re-advertises automatically when the host disconnects, and bonds
survive reboots — after the first pairing the computer reconnects on its own.

## Build and Flash

This is a PlatformIO project.

### Option 1: PlatformIO Core

Install PlatformIO if needed:

```sh
pip install platformio
```

Build from the project folder:

```sh
pio run -e lilygo_t_hmi
```

Upload to the board:

```sh
pio run -e lilygo_t_hmi -t upload
```

If your board is on a specific port, list the ports first:

```sh
pio device list
```

Then upload with the port explicitly:

```sh
pio run -e lilygo_t_hmi -t upload --upload-port COM5
```

Replace `COM5` with your board's port (for example `/dev/ttyACM0` on Linux or
`/dev/cu.usbmodem...` on macOS).

### Option 2: VS Code

1. Install the PlatformIO IDE extension.
2. Open this folder in VS Code.
3. Use **PlatformIO > Build** to compile.
4. Use **PlatformIO > Upload** to flash the board.

### If Upload Fails

If the upload tool cannot connect:

1. Hold the **BOOT** button.
2. Tap the **RST** button once.
3. Release **BOOT**.
4. Run the upload command again.

On Windows, the board can expose two serial ports. Try the other port if the
first one does not respond. The bootloader port is usually the one that appears
after the BOOT + RST sequence.

## How It Works

The firmware advertises as a BLE HID keyboard named `T-HMI Deck`. The host
computer pairs once (bonding is stored in NVS on the board, so it survives
reboots) and reconnects automatically afterwards. Touch presses are handled by
LVGL, queued to a dedicated HID task, and sent as standard BLE HID keyboard
and consumer-control reports.

Pairing notes that matter if you re-flash or modify the firmware:

- The deck keeps its factory Bluetooth address. Do **not** set a custom random
  address; bonding requires the advertised address and the SMP identity
  address to match, and a mismatch makes Windows cycle connect/disconnect.
- The library's default security (`SC_MITM_BOND`) is overridden with plain
  bonding (`ESP_LE_AUTH_BOND`) and `ESP_IO_CAP_NONE`. MITM is impossible
  without a passkey display/keypad and is what breaks Windows pairing.
- The firmware never erases bonds by itself. If the host holds a pairing the
  deck no longer recognises, remove the device in the host's Bluetooth
  settings once and pair again.

## Using the Deck

- Swipe left/right on the button area to change pages.
- Tap the left/right header arrows to change pages (they wrap around).
- Tap a button to send its action; the status bar shows what was sent.
- If no device is connected, taps show **Not connected** in red.
- The status bar shows the Bluetooth state at all times:
  - **BT: Pairing** (amber) — advertising, waiting for a host
  - **BT: Connected** (green) — a host is paired
- The screen dims after 30 seconds without touch input. Tap to wake it.

## Testing

The configuration tests live in `test/test_config`.

Compile-check them without uploading or running:

```sh
pio test -e lilygo_t_hmi --without-uploading --without-testing
```

Run them on a connected board:

```sh
pio test -e lilygo_t_hmi
```

Running the tests flashes a test firmware. Reflash the main application
afterward.

## Customizing Buttons and Pages

All page and button definitions live in `include/config.h`. The deck name is
there too (`BLE_DEVICE_NAME`).

Each button has:

```cpp
struct ButtonDef {
    const char *label;
    const char *icon;
    ButtonAction action;
};
```

### Action types

**Keyboard shortcut** — `Keys(modifiers, key)`. Modifiers are the `MOD_*`
bitmask; the key is a BLE-Keyboard library constant or a plain character:

```cpp
Keys(MOD_CTRL, 'c')                     // Ctrl+C
Keys(MOD_CTRL | MOD_SHIFT, 'm')         // Ctrl+Shift+M
Keys(MOD_GUI, 'd')                      // Win+D
Keys(MOD_GUI | MOD_SHIFT, 's')          // Win+Shift+S
Keys(MOD_NONE, KEY_F13)                 // F13
Keys(MOD_ALT, KEY_TAB)                  // Alt+Tab
Keys(MOD_GUI, 0)                        // Win key alone (Start)
```

**Media key** — `Media(MediaCode::...)`:

```cpp
Media(MediaCode::PLAY_PAUSE)
Media(MediaCode::NEXT_TRACK)
Media(MediaCode::PREV_TRACK)
Media(MediaCode::VOL_UP)
Media(MediaCode::VOL_DOWN)
Media(MediaCode::MUTE)
```

**Sequence** — several timed steps for launchers. Each step can press a
chord, type text, and pause:

```cpp
// Open Windows Terminal: Win+R, wait, type "wt", Enter
static constexpr ActionStep MY_STEPS[] = {
    {MOD_GUI, 'r', nullptr, 500},   // Win+R, wait 500 ms
    {MOD_NONE, 0, "wt", 0},         // type "wt"
    {MOD_NONE, KEY_RETURN, nullptr, 0}, // Enter
};
constexpr ButtonAction MY_ACTION = Sequence(MY_STEPS);
```

Then add the button to a page table and the page to `PAGES`:

```cpp
constexpr ButtonDef MY_BUTTONS[] = {
    {"Mute",  LV_SYMBOL_MUTE,  Media(MediaCode::MUTE)},
    {"Save",  LV_SYMBOL_SAVE,  Keys(MOD_CTRL, 's')},
    {"Task",  LV_SYMBOL_LIST,  Keys(MOD_CTRL | MOD_SHIFT, KEY_ESC)},
};

inline constexpr PageDef PAGES[] = {
    make_page("Media", ACCENT_MEDIA, MEDIA_BUTTONS),
    make_page("Custom", 0x3393E0, MY_BUTTONS), // your own accent color
};
```

The second `make_page` argument is the page's accent color (icons, pressed
borders, and the page dot). Predefined accents: `ACCENT_MEDIA` (lime),
`ACCENT_DESKTOP` (sky), `ACCENT_MEETING` (amber), `ACCENT_OBS` (violet),
`ACCENT_SYSTEM` (red).

`PAGE_COUNT` is calculated automatically. The current limits are:

- Up to 6 buttons per page
- Up to 8 pages

Available modifiers: `MOD_NONE`, `MOD_CTRL`, `MOD_SHIFT`, `MOD_ALT`, `MOD_GUI`
(Windows / Command).

Useful library key constants: `KEY_ESC`, `KEY_TAB`, `KEY_RETURN`,
`KEY_BACKSPACE`, `KEY_DELETE`, `KEY_UP_ARROW` / `KEY_DOWN_ARROW` /
`KEY_LEFT_ARROW` / `KEY_RIGHT_ARROW`, `KEY_F1`–`KEY_F24`.

### Icons and Labels

Icons use LVGL symbols such as:

```cpp
LV_SYMBOL_PLAY
LV_SYMBOL_PAUSE
LV_SYMBOL_NEXT
LV_SYMBOL_PREV
LV_SYMBOL_PLUS
LV_SYMBOL_MINUS
LV_SYMBOL_MUTE
LV_SYMBOL_VOLUME_MAX
LV_SYMBOL_VOLUME_MID
LV_SYMBOL_VIDEO
LV_SYMBOL_AUDIO
LV_SYMBOL_IMAGE
LV_SYMBOL_COPY
LV_SYMBOL_PASTE
LV_SYMBOL_HOME
LV_SYMBOL_SETTINGS
```

Labels are plain text shown under the icon (long names clip with dots).

## OBS Setup

The OBS page sends `F13` through `F18`. To use it:

1. Open OBS.
2. Go to **Settings > Hotkeys**.
3. Find the action you want to control.
4. Click its hotkey field and press the matching key on a normal keyboard, or
   use OBS's key capture after connecting the deck.
5. Repeat for each scene, mic, stream, and replay action.

## Changing Colors and Styling

The visual theme is in:

- `src/ui/theme.h` for the palette
- `src/ui/theme.cpp` for button, label, and header styles
- `src/ui/ui_manager.cpp` for the layout, status bar, and page switching

The current theme uses a near-black background, lime-green accent, and flat
rounded cards with no shadows — the same look as the USB Stream Deck.

## Project Structure

```text
include/                  Configuration, BLE action model, backlight, LVGL config
lib/                      Vendored XPT2046 touch library
src/main.cpp              Display, touch, LVGL, BLE, and application startup
src/ble_hid_manager.*     Bluetooth LE HID keyboard and media output
src/hid_task.*            Tap queue + sender task (keeps the UI loop free)
src/backlight.*           Backlight control and idle dimming
src/debug_log.*           Crash-safe boot log (reset reason + heap per step)
src/ui/                   Theme, swipeable pages, status bar, splash
test/                     On-target configuration validation tests
plans/                    Implementation notes
```

## Troubleshooting

### Capturing the boot log (reboot loops, black screen)

The firmware prints a flush-on-write boot log over USB serial, so lines
survive a crash. Connect the board by USB, start the monitor, then press RST:

```sh
pio device monitor --baud 115200
```

- The first line names the reason of the **previous** reset: `PANIC`
  (exception — the monitor's exception decoder prints the backtrace),
  `TASK_WDT` (a task starved the watchdog), `BROWNOUT` (power dip — try a
  different USB port/cable or a powered hub), `SW_RESET`, or `POWERON`.
- Every `[BOOT]` line carries the internal-heap state (free / minimum-ever /
  largest block) at that exact step, so heap exhaustion during Bluetooth
  init is visible where it happens.
- A `[BOOT] alive Ns (heartbeat M)` line every 5 seconds proves the main
  loop is running; the step logs show which init stage preceded a reset.
- If the screen flashes the splash in a reboot loop, the reset reason is
  also shown on the splash itself (`reboot: TASK_WDT`, ...), so a photo is
  enough when no serial monitor is attached.

### The deck does not appear in Bluetooth settings

- Make sure no other device is currently connected to it — the deck advertises
  one connection at a time. Disconnect it elsewhere first.
- Check the serial monitor: `[BLE]` lines show pairing and connection events.

### Pairing loops (connects and disconnects repeatedly)

Remove **T-HMI Deck** in the computer's Bluetooth settings, restart Bluetooth,
and pair again. The deck never changes its address or erases bonds on its own,
so a stale host-side bond is the usual cause.

### Buttons show "Not connected"

The status bar is red — pair the computer first (see Quick Start). The deck
re-advertises automatically after a disconnect.

### Keyboard shortcuts do nothing on the computer

- Some shortcuts are app-specific (Meetings page); confirm the shortcut in
  your app's settings.
- The Terminal button targets Windows (Win+R → `wt`). On Linux/macOS, remap
  or remove it in `include/config.h`.

### Upload cannot connect

Put the board into bootloader mode: hold **BOOT**, tap **RST**, release
**BOOT**, then upload again.

### Display is dark

The backlight dims after 30 seconds of inactivity. Tap the screen to wake it.

### Touch is inaccurate

Touch calibration is configured in `src/main.cpp`:

```cpp
touch.setCal(1788, 285, 1877, 311, PANEL_WIDTH, PANEL_HEIGHT);
touch.setRotation(TOUCH_ROTATION);
```

The firmware can print `[TOUCH]` diagnostics over serial when a touch starts.
Enable them by changing `-D TOUCH_DEBUG=0` to `-D TOUCH_DEBUG=1` in
`platformio.ini`, rebuild, and open the serial monitor:

```sh
pio device monitor --baud 115200
```

## Safety and Security

The deck is a BLE HID keyboard. Only a paired/bonded host can send it input,
but anyone with physical access to the board can trigger the configured
shortcuts, and a Bluetooth keyboard is inherently wireless — do not bind
destructive actions to buttons that are easy to press accidentally.

The default configuration includes `Win+L` (lock) on the System page. Move or
remove that action if accidental lockouts are a concern.

## Notes

- The display is configured for landscape mode using MADCTL `0xA0`.
- The touch controller uses rotation `1` and the calibration values in
  `src/main.cpp`.
- USB stays in CDC (serial) mode so flashing and the serial monitor work the
  same way as the USB Stream Deck project; the deck output itself is BLE.
- The project uses the LilyGo T-HMI pinout and is not intended as a generic
  ESP32-S3 display example without checking the wiring.
