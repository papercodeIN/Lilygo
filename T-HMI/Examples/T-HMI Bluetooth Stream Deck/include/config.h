/**
 * @file config.h
 * USER-EDITABLE: pages, buttons and the Bluetooth HID actions they send.
 *
 * Single source of truth for the whole deck. Add a page or remap a button
 * here; the UI is built from this table at boot.
 *
 * Layout: landscape 320x240, 3 columns x 2 rows, up to 6 buttons per page.
 * The user swipes left/right through the pages (or taps the header arrows).
 * Bad page layouts (empty page, too many buttons, missing label/icon) fail the
 * build through static_assert. Actions are validated once at boot because the
 * Arduino GCC 8.4 toolchain cannot constant-evaluate the action table.
 */
#pragma once

#include <cstddef>
#include <cstdint>

#include <lvgl.h>

#include "actions.h"

// ---------------------------------------------------------------------------
// Deck identity
// ---------------------------------------------------------------------------

// The name the deck advertises over Bluetooth. Pair the computer with this
// name; it also shows on the splash screen.
inline constexpr const char *BLE_DEVICE_NAME = "T-HMI Deck";
inline constexpr const char *BLE_MANUFACTURER = "Fusion Automate";

// ---------------------------------------------------------------------------
// Button / page model
// ---------------------------------------------------------------------------

struct ButtonDef {
    const char *label; // text under the icon
    const char *icon;  // LVGL symbol, e.g. LV_SYMBOL_PLAY
    ButtonAction action;
};

constexpr size_t MAX_BUTTONS_PER_PAGE = 6;
constexpr size_t MAX_PAGES = 8;

struct PageDef {
    const char *title;
    uint32_t accent; // page accent color (icons, pressed border, page dot)
    ButtonDef buttons[MAX_BUTTONS_PER_PAGE];
    size_t button_count;
};

template <size_t N>
constexpr PageDef make_page(const char *title, uint32_t accent, const ButtonDef (&buttons)[N]) {
    static_assert(N > 0, "config.h: a page needs at least one button");
    static_assert(N <= MAX_BUTTONS_PER_PAGE, "config.h: too many buttons for a page");
    PageDef page{title, accent, {}, N};
    for (size_t i = 0; i < N; ++i) page.buttons[i] = buttons[i];
    return page;
}

// ---------------------------------------------------------------------------
// Sequences
// ---------------------------------------------------------------------------

// Open Windows Terminal: Win+R, wait for the Run dialog, type "wt", Enter.
static constexpr ActionStep TERMINAL_STEPS[] = {
    {MOD_GUI, 'r', nullptr, 500},
    {MOD_NONE, 0, "wt", 0},
    {MOD_NONE, KEY_RETURN, nullptr, 0},
};

inline constexpr ButtonAction SEQUENCE_TERMINAL = Sequence(TERMINAL_STEPS);

// ---------------------------------------------------------------------------
// PAGE TABLE — edit freely. Keys use the ESP32-BLE-Keyboard constants
// (KEY_LEFT_*, KEY_F13, ... or plain characters like 'c').
// ---------------------------------------------------------------------------

constexpr ButtonDef MEDIA_BUTTONS[] = {
    {"Play",  LV_SYMBOL_PLAY,        Media(MediaCode::PLAY_PAUSE)},
    {"Next",  LV_SYMBOL_NEXT,        Media(MediaCode::NEXT_TRACK)},
    {"Prev",  LV_SYMBOL_PREV,        Media(MediaCode::PREV_TRACK)},
    {"Vol +", LV_SYMBOL_VOLUME_MAX,  Media(MediaCode::VOL_UP)},
    {"Vol -", LV_SYMBOL_VOLUME_MID,  Media(MediaCode::VOL_DOWN)},
    {"Mute",  LV_SYMBOL_MUTE,        Media(MediaCode::MUTE)},
};

// Windows desktop shortcuts. "Start" is a modifier-only tap (Win key).
constexpr ButtonDef DESKTOP_BUTTONS[] = {
    {"Start",   LV_SYMBOL_BARS,    Keys(MOD_GUI, 0)},                        // Win
    {"Alt+Tab", LV_SYMBOL_LIST,    Keys(MOD_ALT, KEY_TAB)},                  // Alt+Tab
    {"Snip",    LV_SYMBOL_IMAGE,   Keys(MOD_GUI | MOD_SHIFT, 's')},          // Win+Shift+S
    {"Desktop", LV_SYMBOL_HOME,    Keys(MOD_GUI, 'd')},                      // Win+D
    {"Copy",    LV_SYMBOL_COPY,    Keys(MOD_CTRL, 'c')},                     // Ctrl+C
    {"Paste",   LV_SYMBOL_PASTE,   Keys(MOD_CTRL, 'v')},                     // Ctrl+V
};

// Default Teams/Zoom/Meet combos; remap to taste.
constexpr ButtonDef MEETING_BUTTONS[] = {
    {"Mic",   LV_SYMBOL_AUDIO,    Keys(MOD_CTRL | MOD_SHIFT, 'm')},          // Ctrl+Shift+M
    {"Cam",   LV_SYMBOL_VIDEO,    Keys(MOD_CTRL | MOD_SHIFT, 'o')},          // Ctrl+Shift+O
    {"Hand",  LV_SYMBOL_OK,       Keys(MOD_ALT, 'y')},                       // Alt+Y
    {"React", LV_SYMBOL_BELL,     Keys(MOD_CTRL | MOD_SHIFT, 'p')},          // Ctrl+Shift+P
    {"Leave", LV_SYMBOL_CLOSE,    Keys(MOD_CTRL | MOD_SHIFT, 'h')},          // Ctrl+Shift+H
    {"Emoji", LV_SYMBOL_SETTINGS, Keys(MOD_GUI, '.')},                       // Win+.
};

// F13-F18: bind them in OBS under Settings > Hotkeys to avoid conflicts.
constexpr ButtonDef OBS_BUTTONS[] = {
    {"Scene 1", LV_SYMBOL_VIDEO,   Keys(MOD_NONE, KEY_F13)},
    {"Scene 2", LV_SYMBOL_VIDEO,   Keys(MOD_NONE, KEY_F14)},
    {"Scene 3", LV_SYMBOL_VIDEO,   Keys(MOD_NONE, KEY_F15)},
    {"Mic",     LV_SYMBOL_AUDIO,   Keys(MOD_NONE, KEY_F16)},
    {"Stream",  LV_SYMBOL_PLAY,    Keys(MOD_NONE, KEY_F17)},
    {"Replay",  LV_SYMBOL_REFRESH, Keys(MOD_NONE, KEY_F18)},
};

constexpr ButtonDef SYSTEM_BUTTONS[] = {
    {"Terminal", LV_SYMBOL_KEYBOARD, SEQUENCE_TERMINAL},
    {"Esc",      LV_SYMBOL_CLOSE,    Keys(MOD_NONE, KEY_ESC)},
    {"Ctrl+Tab", LV_SYMBOL_RIGHT,    Keys(MOD_CTRL, KEY_TAB)},                // Ctrl+Tab
    {"Undo",     LV_SYMBOL_LEFT,     Keys(MOD_CTRL, 'z')},                    // Ctrl+Z
    {"Redo",     LV_SYMBOL_REFRESH,  Keys(MOD_CTRL, 'y')},                    // Ctrl+Y
    {"Lock",     LV_SYMBOL_EYE_CLOSE, Keys(MOD_GUI, 'l')},                     // Win+L
};

// ---------------------------------------------------------------------------
// Page table assembly
// ---------------------------------------------------------------------------

// Page accent colors — one hue per page so the user can tell pages apart at
// a glance. Same family as the CrowPanel deck palette.
inline constexpr uint32_t ACCENT_MEDIA = 0x4ADE4A;   // lime
inline constexpr uint32_t ACCENT_DESKTOP = 0x38BDF8; // sky
inline constexpr uint32_t ACCENT_MEETING = 0xFCBB00; // amber
inline constexpr uint32_t ACCENT_OBS = 0xA78BFA;     // violet
inline constexpr uint32_t ACCENT_SYSTEM = 0xFB2C36;  // red

inline constexpr PageDef PAGES[] = {
    make_page("Media", ACCENT_MEDIA, MEDIA_BUTTONS),
    make_page("Desktop", ACCENT_DESKTOP, DESKTOP_BUTTONS),
    make_page("Meetings", ACCENT_MEETING, MEETING_BUTTONS),
    make_page("OBS", ACCENT_OBS, OBS_BUTTONS),
    make_page("System", ACCENT_SYSTEM, SYSTEM_BUTTONS),
};

inline constexpr size_t PAGE_COUNT = sizeof(PAGES) / sizeof(PAGES[0]);

static_assert(PAGE_COUNT > 0, "config.h: at least one page is required");
static_assert(PAGE_COUNT <= MAX_PAGES, "config.h: too many pages configured");

// ---------------------------------------------------------------------------
// Compile-time validation
// ---------------------------------------------------------------------------

// Layout checks are evaluated at compile time. The GCC 8.4 toolchain shipped
// with the Arduino core cannot constant-evaluate reads of ButtonAction members
// through the page table, so action checks run once at boot instead.
constexpr bool layout_is_valid(const PageDef &page) {
    if (page.title == nullptr) return false;
    if (page.button_count == 0 || page.button_count > MAX_BUTTONS_PER_PAGE) return false;
    for (size_t i = 0; i < page.button_count; ++i) {
        if (page.buttons[i].label == nullptr || page.buttons[i].icon == nullptr) return false;
    }
    return true;
}

constexpr bool all_layouts_are_valid() {
    for (const PageDef &page : PAGES) {
        if (!layout_is_valid(page)) return false;
    }
    return true;
}

static_assert(all_layouts_are_valid(), "config.h: invalid page layout");

// Call once at boot; see setup() in main.cpp.
inline bool config_is_valid() {
    for (const PageDef &page : PAGES) {
        if (!layout_is_valid(page)) return false;
        for (size_t i = 0; i < page.button_count; ++i) {
            if (!action_is_valid(page.buttons[i].action)) return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Behavior settings
// ---------------------------------------------------------------------------

constexpr uint32_t IDLE_DIM_TIMEOUT_MS = 30000; // dim the screen after idle
constexpr uint32_t SPLASH_DURATION_MS = 1500;

// BLE timing
constexpr uint32_t HID_CHORD_MS = 80;   // key-down hold for modifier chords
constexpr uint32_t BLE_POLL_MS = 500;   // connection-state poll interval

static_assert(IDLE_DIM_TIMEOUT_MS > 0, "config.h: idle timeout must be positive");
static_assert(SPLASH_DURATION_MS > 0, "config.h: splash duration must be positive");
static_assert(HID_CHORD_MS > 0, "config.h: chord hold must be positive");
static_assert(BLE_POLL_MS > 0, "config.h: BLE poll interval must be positive");
