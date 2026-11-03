/**
 * @file hid_task.h
 * FreeRTOS task that performs the blocking BLE HID sends so the LVGL loop
 * never stalls on timed chords and sequences.
 */
#pragma once

#include "config.h"

namespace hid_task {

// Creates the tap queue and the sender task. Call once from setup().
void start();

// Queues one button press. Safe to call from the LVGL loop; never blocks.
// Posts UI feedback when the queue is full.
void request(const ButtonDef *definition);

} // namespace hid_task
