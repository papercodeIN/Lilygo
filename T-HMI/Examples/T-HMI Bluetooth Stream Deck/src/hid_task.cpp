/**
 * @file hid_task.cpp
 * Consumes tap requests and performs the BLE HID sends off the LVGL loop.
 */

#include "hid_task.h"

#include <Arduino.h>
#include <FreeRTOS.h>
#include <queue.h>

#include "ble_hid_manager.h"
#include "ui/ui_manager.h"

namespace {

constexpr size_t TAP_QUEUE_LEN = 8;
constexpr size_t HID_TASK_STACK_WORDS = 8 * 1024 / sizeof(StackType_t);

struct TapRequest {
    const ButtonDef *definition;
};

QueueHandle_t tap_queue = nullptr;

void hid_loop(void *) {
    TapRequest req;
    for (;;) {
        if (xQueueReceive(tap_queue, &req, portMAX_DELAY) != pdTRUE) continue;
        if (req.definition == nullptr) continue;

        Serial.printf("[HID] %s\n", req.definition->label);
        switch (BLEHID.send(req.definition->action)) {
        case SendStatus::SENT:
            ui::post_feedback(req.definition->label, ui::Feedback::OK);
            break;
        case SendStatus::NOT_CONNECTED:
            ui::post_feedback("Not connected", ui::Feedback::ERROR);
            break;
        }
    }
}

} // namespace

namespace hid_task {

void start() {
    if (tap_queue != nullptr) return;
    tap_queue = xQueueCreate(TAP_QUEUE_LEN, sizeof(TapRequest));
    if (tap_queue == nullptr) {
        Serial.println("[HID] tap queue creation failed");
        return;
    }
    xTaskCreatePinnedToCore(hid_loop, "hid", HID_TASK_STACK_WORDS, nullptr, 1, nullptr, 0);
}

void request(const ButtonDef *definition) {
    if (tap_queue == nullptr || definition == nullptr) return;
    TapRequest req{definition};
    if (xQueueSend(tap_queue, &req, 0) != pdTRUE) {
        Serial.printf("[HID] tap queue full, dropped %s\n", definition->label);
        ui::post_feedback("Busy", ui::Feedback::WARNING);
    }
}

} // namespace hid_task
