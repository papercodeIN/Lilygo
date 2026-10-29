#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <vector>
#include "audio/Mp3Player.h"
#include "bt/BridgeLink.h"
#include "power/PowerManager.h"
#include "ui/Ui.h"

// =============================================================================
//  Application core: owns the UI, player, bridge link and power manager,
//  routes events between them and runs the screen state machine.
//
//  Design note (per requirements): the device NEVER auto-reconnects to the
//  previously used Bluetooth device. Every boot lands on the device screen
//  with a fresh scan; the user taps a device to connect.
// =============================================================================

class LilyGo_Class;

class AppClass {
public:
    void begin(LilyGo_Class &device);
    void tick();  // loop task: bridge UART polling

private:
    enum class Screen : uint8_t { NONE, DEVICES, PLAYER };

    // ---- boot ----
    bool scanSdCard();
    static String stripExtension(const String &name);
    static bool iequalsSuffix(const String &name, const char *suffix);

    // ---- LVGL task ----
    static void lvglTaskTrampoline(void *arg);
    static void uiPollTrampoline(lv_timer_t *timer);
    void uiPoll();
    void applyBrightness();

    // ---- loop-task event flags (bridge callbacks) ----
    static void bridgeAliveCb(bool up);
    static void bridgeScanItemCb(const BridgeScanItem &item);
    static void bridgeScanDoneCb();
    static void bridgeStatusCb(uint8_t state, const char *name);
    static void bridgeAvrcpCb(uint8_t key);
    void handleStatus(uint8_t state, const char *name);

    // ---- UI callbacks (LVGL task) ----
    static void uiDeviceTappedCb(int listIndex);
    static void uiScanAgainCb();
    static void uiPlayerBackCb();
    static void uiTogglePlayCb();
    static void uiNextCb();
    static void uiPrevCb();
    static void uiSeekCb(uint32_t ms);
    static void uiVolumeCb(uint8_t v);
    void connectDevice(int listIndex);
    void setVolume(uint8_t v);   // deferred NVS write, mirrored to UI + bridge

    void postToast(const char *msg);   // thread-safe -> shown by uiPoll
    void requestScreen(Screen s);      // thread-safe -> applied by uiPoll

    std::vector<TrackItem> tracks;
    std::vector<BridgeScanItem> devices;
    SemaphoreHandle_t listMutex = nullptr;

    Ui ui;
    Mp3Player player;
    PowerManager power;

    Preferences prefs;
    uint8_t volume = 80;
    uint32_t lastVolumeChangeMs = 0;

    // volatile flags shared between tasks
    volatile Screen pendingScreen = Screen::NONE;
    volatile bool devicesChanged = false;
    volatile uint8_t bridgeState = BRP_LINK_IDLE;
    volatile bool scanning = false;
    volatile int connectingIdx = -1;
    char toastBuf[96] = {0};
    volatile bool toastPending = false;
    char connectingName[40] = {0};

    uint8_t currentBrightness = 200;
    uint8_t lastVolumeShown = 255;
    bool lowBatteryWarned = false;
    uint32_t lastBatteryPollMs = 0;
};

extern AppClass App;
