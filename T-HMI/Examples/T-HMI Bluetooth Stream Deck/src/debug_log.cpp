/**
 * @file debug_log.cpp
 * Implementation of the crash-safe boot logger.
 */

#include "debug_log.h"

#include <Arduino.h>

#include <esp_heap_caps.h>
#include <esp_system.h>

#include <cstdarg>
#include <cstdio>

namespace dbg {

namespace {

const char *name_for_reset(esp_reset_reason_t reason) {
    switch (reason) {
    case ESP_RST_POWERON:
        return "POWERON";
    case ESP_RST_SW:
        return "SW_RESET (esp_restart/abort)";
    case ESP_RST_PANIC:
        return "PANIC (exception)";
    case ESP_RST_INT_WDT:
        return "INT_WDT (interrupt watchdog)";
    case ESP_RST_TASK_WDT:
        return "TASK_WDT (task watchdog)";
    case ESP_RST_WDT:
        return "OTHER_WDT";
    case ESP_RST_BROWNOUT:
        return "BROWNOUT (power dip)";
    case ESP_RST_EXT:
        return "EXT (reset pin)";
    case ESP_RST_SDIO:
        return "SDIO";
    default:
        return "UNKNOWN";
    }
}

} // namespace

const char *reset_reason_name() {
    return name_for_reset(esp_reset_reason());
}

void log(const char *fmt, ...) {
    char message[160];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    const size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    const size_t min_internal = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
    const size_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    Serial.printf("[BOOT] %s | int free %u (min %u) largest %u\n", message,
                  static_cast<unsigned>(free_internal), static_cast<unsigned>(min_internal),
                  static_cast<unsigned>(largest_block));
    // Without this, output buffered in the USB CDC stack is lost when the
    // device panics or resets right after the print.
    Serial.flush();
}

} // namespace dbg
