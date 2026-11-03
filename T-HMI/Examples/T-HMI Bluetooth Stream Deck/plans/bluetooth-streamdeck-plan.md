# T-HMI Bluetooth Stream Deck — implementation plan

Reference projects:

- `T-HMI USB Stream Deck` (same folder tree) — T-HMI display/touch bring-up,
  theme, config-driven page table, idle dimming, project layout.
- CrowPanel 7" `Bluetooth_Stream_Deck` — BLE HID workflow: ESP32-BLE-Keyboard
  security fix, connection-status UI, HID task off the UI loop.

## Goal

The USB deck works only while cabled. This project sends the same class of
actions (media keys, keyboard shortcuts, small sequences) over Bluetooth LE
HID so any PC with Bluetooth can use the deck. The 320x240 panel is much
smaller than the CrowPanel 7", so buttons live on multiple swipeable pages
instead of one large 4x4 grid.

## Architecture

```
touch → LVGL (loop task) ─ tap → hid_task (core 0) → BleHIDManager → host
                ▲ feedback (critical-section slot, drained by an LVGL timer)
                └ BLE poll timer → BLEHID.connected() → status bar
```

- **include/actions.h** — action model: `Keys(mods, key)`, `Media(code)`,
  `Sequence(steps)`. Keys use the BLE keyboard library's constants
  (`KEY_LEFT_*`, `KEY_F13`, `KEY_TAB`, ASCII chars); mods are a `MOD_*` mask.
- **include/config.h** — pages/buttons table (same shape as the USB deck),
  per-page accent colors, BLE device name, timing constants.
- **src/ble_hid_manager.*` — wraps ESP32-BLE-Keyboard. `onStarted` overrides
  the library's `ESP_LE_AUTH_REQ_SC_MITM_BOND` with `ESP_LE_AUTH_BOND` +
  `ESP_IO_CAP_NONE` (Just Works). MITM is impossible without a passkey I/O
  and a MITM requirement makes Windows abort pairing in a connect loop.
  The library re-advertises on disconnect (verified in its Bluedroid
  `onDisconnect`).
- **src/hid_task.*` — FreeRTOS task + tap queue so timed chords/sequences
  never block the LVGL loop.
- **src/ui/ui_manager.cpp** — header (arrows + title + page dots), status bar
  (BLE state left, action feedback right), tileview with one tile per page.
  Swipe left/right or tap the arrows. Page changes are tracked through the
  tileview's own `LV_EVENT_VALUE_CHANGED` (the tileview updates `tile_act`
  on every snap) with the arrows setting the tile directly.

## Key decisions

- **All LVGL geometry is computed inside `build()`** — `LV_VER_RES`/`LV_HOR_RES`
  are runtime calls that return 0 before a display is registered. An early
  draft cached `CONTENT_HEIGHT` as a namespace-scope constant, which resolved
  to -64 px before boot: every button collapsed to nothing (invisible UI) and
  scrolling the dead tileview panicked the device. Do not hoist any
  `LV_*_RES` math out of `build()`.
- **Tileview over gestures**: native scroll-snap gives animated page slides
  and correctly suppresses button clicks once a drag turns into a scroll.
  Buttons act on `LV_EVENT_CLICKED` (fires on release without scrolling).
- **Transparent tileview over a static tech grid**: the grid lines are
  siblings behind the tileview, so pages slide over a fixed technical
  backdrop (one-time draw cost).
- **Per-page accents** (`PageDef.accent`): icons, pressed borders, and the
  active page dot take the page's hue — visual orientation without noise.
- **USB config kept** (`ARDUINO_USB_MODE=0`, CDC on boot): identical flash
  and serial-monitor workflow to the USB deck project; deck output is BLE.
- **Media reports via a constexpr table** (`MEDIA_REPORTS` + `MediaCode`):
  the library's `KEY_MEDIA_*` constants are non-constexpr globals, so the
  constexpr page table can't read them; the table is verified against the
  library once at boot (`media_reports_in_sync_with_library()`).
- **Factory public address, bonds in NVS**: never changed by firmware
  (matching the CrowPanel findings about SMP identity addresses).
- **LVGL 8.3.11 + espressif32@6.9.0**: same versions as the USB deck.

## Tooling notes (PlatformIO 6.2.0 + Python 3.13)

- The `unity` test runner injects `throwtheswitch/Unity@^2.6.1` per run and
  cleans it up afterwards; combined with an SCons 4.11.1 bug
  (`get_max_drift_csig()` does `time.time() - None` when a file vanishes
  mid-scan), test builds could crash. The package now pins the identical
  spec in `lib_deps` (no install/remove churn), and the local
  `tool-scons` install carries a one-line `mtime is not None` guard in
  `SCons/Node/FS.py`. Reinstalling `tool-scons` reverts the guard; the
  `lib_deps` pin alone keeps builds stable.

## Layout (320x240)

- Header 44 px: ◀ / title / page dots / ▶
- Status bar 20 px: BLE state (left), last action (right)
- Tileview 176 px: 3x2 button grid per page, 5 pages by default
  (Media, Desktop, Meetings, OBS, System), over a faint 32 px tech grid
