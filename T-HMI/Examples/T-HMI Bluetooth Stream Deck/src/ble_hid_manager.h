/**
 * @file ble_hid_manager.h
 * Bluetooth LE HID output: keyboard chords, media keys and sequences via the
 * ESP32-BLE-Keyboard library.
 */
#pragma once

#include <cstdint>

#include "actions.h"

enum class SendStatus : uint8_t {
    SENT,
    NOT_CONNECTED,
};

class BleHIDManager {
public:
    // Initializes the BLE stack, configures pairing security and starts
    // advertising as BLE_DEVICE_NAME. Blocks for a moment while Bluedroid
    // starts; call once from setup().
    void begin();

    // True when a host has an active HID connection.
    bool connected() const;

    // Sends one action. Blocks (chords and sequences are timed), so call this
    // from the HID task, never from the LVGL loop.
    SendStatus send(const ButtonAction &action);

private:
    void press_chord(uint8_t mods, uint8_t key);
};

extern BleHIDManager BLEHID;
