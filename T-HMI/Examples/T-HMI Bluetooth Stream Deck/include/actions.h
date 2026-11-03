/**
 * @file actions.h
 * BLE HID action model shared by the configuration table and the HID manager.
 *
 * Keys are expressed with the ESP32-BLE-Keyboard library's own constants
 * (KEY_LEFT_*, KEY_F13, KEY_TAB, ... or a plain ASCII character), so a button
 * definition reads exactly like the library calls it needs. Modifier bits are
 * our own MOD_* mask; BleHIDManager translates each bit to the matching
 * library modifier key.
 */
#pragma once

#include <cstddef>
#include <cstdint>

#include <BleKeyboard.h> // KEY_* constants; included only so the media table
                         // below can be verified against it at boot

// Modifier bitmask. BleHIDManager presses one library modifier key per bit.
enum Mod : uint8_t {
    MOD_NONE = 0x00,
    MOD_CTRL = 0x01,
    MOD_SHIFT = 0x02,
    MOD_ALT = 0x04,
    MOD_GUI = 0x08, // Windows / Command key
};

constexpr uint8_t MOD_ALL =
    static_cast<uint8_t>(MOD_CTRL | MOD_SHIFT | MOD_ALT | MOD_GUI);

constexpr bool is_valid_mod_mask(uint8_t mods) {
    return (mods & static_cast<uint8_t>(~MOD_ALL)) == 0;
}

// One action kind.
enum class ActionKind : uint8_t {
    NONE = 0,
    CHORD,   // hold modifiers + tap one key
    MEDIA,   // consumer control (play/pause, volume, ...)
    SEQUENCE, // several timed steps, e.g. Win+R, wait, type "wt", Enter
};

// A single SEQUENCE step: chord (optional), typed text (optional), pause.
struct ActionStep {
    uint8_t mods;      // MOD_* bitmask, MOD_NONE if none
    uint8_t key;       // library key constant or ASCII, 0 for none
    const char *text;  // string to type, nullptr for none
    uint16_t pause_ms; // pause after this step finishes
};

// One button action. Only the field(s) of `kind` are meaningful.
struct ButtonAction {
    ActionKind kind;
    // CHORD
    uint8_t mods; // MOD_* bitmask
    uint8_t key;  // library key constant or ASCII character
    // MEDIA (the library's 2-byte media report, copied so the table stays
    // valid without referencing library globals at runtime)
    uint8_t media0;
    uint8_t media1;
    // SEQUENCE
    const ActionStep *steps;
    uint8_t step_count;
};

constexpr ButtonAction Keys(uint8_t mods, uint8_t keycode) {
    return ButtonAction{ActionKind::CHORD, mods, keycode, 0, 0, nullptr, 0};
}

// Media actions. The ESP32-BLE-Keyboard library's KEY_MEDIA_* constants are
// plain globals, so a constexpr page table cannot read them directly; this
// table mirrors their 2-byte report bitmaps and is checked against the
// library's own constants below.
enum class MediaCode : uint8_t {
    NONE = 0,
    PLAY_PAUSE,
    NEXT_TRACK,
    PREV_TRACK,
    STOP,
    MUTE,
    VOL_UP,
    VOL_DOWN,
};

struct MediaReport {
    uint8_t b0;
    uint8_t b1;
};

inline constexpr MediaReport MEDIA_REPORTS[] = {
    {0, 0},   // NONE
    {8, 0},   // PLAY_PAUSE  == KEY_MEDIA_PLAY_PAUSE
    {1, 0},   // NEXT_TRACK  == KEY_MEDIA_NEXT_TRACK
    {2, 0},   // PREV_TRACK  == KEY_MEDIA_PREVIOUS_TRACK
    {4, 0},   // STOP        == KEY_MEDIA_STOP
    {16, 0},  // MUTE        == KEY_MEDIA_MUTE
    {32, 0},  // VOL_UP      == KEY_MEDIA_VOLUME_UP
    {64, 0},  // VOL_DOWN    == KEY_MEDIA_VOLUME_DOWN
};

// The library's KEY_MEDIA_* constants are plain const arrays (not constexpr),
// so the table above cannot be static_assert-checked against them. This runs
// once at boot alongside config_is_valid().
inline bool media_reports_in_sync_with_library() {
    const MediaReport &play_pause = MEDIA_REPORTS[static_cast<size_t>(MediaCode::PLAY_PAUSE)];
    if (play_pause.b0 != KEY_MEDIA_PLAY_PAUSE[0] || play_pause.b1 != KEY_MEDIA_PLAY_PAUSE[1]) return false;
    const MediaReport &next = MEDIA_REPORTS[static_cast<size_t>(MediaCode::NEXT_TRACK)];
    if (next.b0 != KEY_MEDIA_NEXT_TRACK[0] || next.b1 != KEY_MEDIA_NEXT_TRACK[1]) return false;
    const MediaReport &prev = MEDIA_REPORTS[static_cast<size_t>(MediaCode::PREV_TRACK)];
    if (prev.b0 != KEY_MEDIA_PREVIOUS_TRACK[0] || prev.b1 != KEY_MEDIA_PREVIOUS_TRACK[1]) return false;
    const MediaReport &stop = MEDIA_REPORTS[static_cast<size_t>(MediaCode::STOP)];
    if (stop.b0 != KEY_MEDIA_STOP[0] || stop.b1 != KEY_MEDIA_STOP[1]) return false;
    const MediaReport &mute = MEDIA_REPORTS[static_cast<size_t>(MediaCode::MUTE)];
    if (mute.b0 != KEY_MEDIA_MUTE[0] || mute.b1 != KEY_MEDIA_MUTE[1]) return false;
    const MediaReport &vol_up = MEDIA_REPORTS[static_cast<size_t>(MediaCode::VOL_UP)];
    if (vol_up.b0 != KEY_MEDIA_VOLUME_UP[0] || vol_up.b1 != KEY_MEDIA_VOLUME_UP[1]) return false;
    const MediaReport &vol_down = MEDIA_REPORTS[static_cast<size_t>(MediaCode::VOL_DOWN)];
    if (vol_down.b0 != KEY_MEDIA_VOLUME_DOWN[0] || vol_down.b1 != KEY_MEDIA_VOLUME_DOWN[1]) return false;
    return true;
}

constexpr ButtonAction Media(MediaCode code) {
    const MediaReport &report = MEDIA_REPORTS[static_cast<size_t>(code)];
    return ButtonAction{ActionKind::MEDIA, MOD_NONE, 0, report.b0, report.b1, nullptr, 0};
}

template <size_t N>
constexpr ButtonAction Sequence(const ActionStep (&steps)[N]) {
    return ButtonAction{ActionKind::SEQUENCE, MOD_NONE, 0, 0, 0, steps,
                        static_cast<uint8_t>(N)};
}

constexpr bool step_is_valid(const ActionStep &step) {
    if (!is_valid_mod_mask(step.mods)) return false;
    return step.mods != MOD_NONE || step.key != 0 || step.text != nullptr ||
           step.pause_ms != 0;
}

constexpr bool action_has_key(const ButtonAction &action) {
    return action.key != 0 || action.mods != MOD_NONE;
}

constexpr bool action_is_valid(const ButtonAction &action) {
    if (!is_valid_mod_mask(action.mods)) return false;
    switch (action.kind) {
    case ActionKind::CHORD:
        return action_has_key(action);
    case ActionKind::MEDIA:
        return action.media0 != 0 || action.media1 != 0;
    case ActionKind::SEQUENCE:
        if (action.steps == nullptr || action.step_count == 0) return false;
        for (size_t i = 0; i < action.step_count; ++i) {
            if (!step_is_valid(action.steps[i])) return false;
        }
        return true;
    case ActionKind::NONE:
    default:
        return false;
    }
}
