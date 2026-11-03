/**
 * @file ui_manager.h
 * Deck UI: status header, swipeable tileview pages, splash screen and idle
 * dimming. Also the thread-safe action-feedback slot used by the HID task.
 */
#pragma once

#include <cstdint>

namespace ui {

enum class Feedback : uint8_t {
    OK,          // action sent (green)
    WARNING,     // e.g. queue full (amber)
    ERROR,       // e.g. not connected (red)
};

void build();
void show_splash(uint32_t duration_ms, const char *boot_notice = nullptr);
void notify_activity();

// Thread-safe one-shot status message for the status bar. Safe to call from
// any task; the LVGL loop picks it up within ~100 ms. `text` must point to
// storage that outlives the call (config strings, string literals).
void post_feedback(const char *text, Feedback kind);

} // namespace ui
