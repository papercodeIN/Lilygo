/**
 * @file ble_hid_manager.cpp
 * BLE HID implementation. The deck keeps its factory public address for life:
 * bonding requires the advertised address and the SMP identity address to
 * match, and Bluedroid always distributes the public one. Bonds are stored in
 * NVS by the stack and survive reboots, so the host reconnects with the stored
 * keys. If the host ever holds a pairing the deck no longer recognises,
 * remove the device in the host's Bluetooth settings once and pair again —
 * the firmware never erases bonds or changes its address on its own.
 */

#include "ble_hid_manager.h"

#include <Arduino.h>
#include <BLEDevice.h> // ESP-IDF BLE + bond APIs (diagnostics logging)
#include <BleKeyboard.h>

#include "config.h"
#include "debug_log.h"

namespace {

constexpr uint32_t ADV_MIN_INTERVAL = 0x20;  // 31.25 ms, HID-friendly
constexpr uint32_t ADV_MAX_INTERVAL = 0x30;  // 46.875 ms
constexpr uint32_t CONN_MIN_INTERVAL = 0x0C; // 15 ms
constexpr uint32_t CONN_MAX_INTERVAL = 0x18; // 30 ms

// Chained over the library's own behaviour. The library defaults to
// ESP_LE_AUTH_REQ_SC_MITM_BOND; MITM is impossible here (no display, no
// keypad for a passkey), and a MITM requirement against a NoInputNoOutput
// device makes hosts abort pairing and cycle connect/disconnect. Bonding
// without MITM (Just Works) pairs cleanly and the bond persists in NVS.
class DeckBleKeyboard : public BleKeyboard {
public:
    DeckBleKeyboard(const std::string &name, const std::string &manufacturer, uint8_t battery)
        : BleKeyboard(name, manufacturer, battery) {}

    void onConnect(BLEServer *server) override {
        BleKeyboard::onConnect(server);
        Serial.println("[BLE] host connected; HID notifications enabled");
    }

    void onDisconnect(BLEServer *server) override {
        BleKeyboard::onDisconnect(server);
        Serial.println("[BLE] host disconnected; advertising restarted");
    }

    void onStarted(BLEServer *server) override {
        uint8_t auth_req = ESP_LE_AUTH_BOND;
        esp_err_t auth_err = esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req,
                                                            sizeof(auth_req));
        uint8_t io_cap = ESP_IO_CAP_NONE;
        esp_err_t io_err = esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &io_cap,
                                                          sizeof(io_cap));
        dbg::log("BLE: security set (auth %s, io %s)", esp_err_to_name(auth_err),
                 esp_err_to_name(io_err));

        BLEAdvertising *adv = server->getAdvertising();
        adv->setMinInterval(ADV_MIN_INTERVAL);
        adv->setMaxInterval(ADV_MAX_INTERVAL);
        adv->setMinPreferred(CONN_MIN_INTERVAL);
        adv->setMaxPreferred(CONN_MAX_INTERVAL);
    }
};

DeckBleKeyboard deck_keyboard(BLE_DEVICE_NAME, BLE_MANUFACTURER, 100);

} // namespace

BleHIDManager BLEHID;

void BleHIDManager::begin() {
    // Initialize Bluedroid under the advertised device name first so bond
    // diagnostics work; the BLE library's own init call is guarded internally.
    dbg::log("BLE: initializing Bluedroid as '%s'", BLE_DEVICE_NAME);
    BLEDevice::init(BLE_DEVICE_NAME);
    dbg::log("BLE: stack up; bonds stored = %d", static_cast<int>(esp_ble_get_bond_device_num()));

    // Creates the HID service and starts advertising with the security
    // settings configured in DeckBleKeyboard::onStarted().
    deck_keyboard.begin();
    dbg::log("BLE: HID service up; advertising started");
}

bool BleHIDManager::connected() const {
    return deck_keyboard.isConnected();
}

SendStatus BleHIDManager::send(const ButtonAction &action) {
    if (!deck_keyboard.isConnected()) return SendStatus::NOT_CONNECTED;

    switch (action.kind) {
    case ActionKind::CHORD: {
        press_chord(action.mods, action.key);
        break;
    }
    case ActionKind::MEDIA: {
        const MediaKeyReport report = {action.media0, action.media1};
        deck_keyboard.write(report);
        break;
    }
    case ActionKind::SEQUENCE: {
        for (size_t i = 0; i < action.step_count; ++i) {
            const ActionStep &step = action.steps[i];
            if (step.mods != MOD_NONE || step.key != 0) press_chord(step.mods, step.key);
            if (step.text != nullptr) deck_keyboard.print(step.text);
            if (step.pause_ms != 0) delay(step.pause_ms);
        }
        break;
    }
    case ActionKind::NONE:
        return SendStatus::NOT_CONNECTED;
    }

    return SendStatus::SENT;
}

void BleHIDManager::press_chord(uint8_t mods, uint8_t key) {
    if (mods & MOD_CTRL) deck_keyboard.press(KEY_LEFT_CTRL);
    if (mods & MOD_SHIFT) deck_keyboard.press(KEY_LEFT_SHIFT);
    if (mods & MOD_ALT) deck_keyboard.press(KEY_LEFT_ALT);
    if (mods & MOD_GUI) deck_keyboard.press(KEY_LEFT_GUI);
    if (key != 0) deck_keyboard.press(key);
    delay(HID_CHORD_MS);
    deck_keyboard.releaseAll();
}
