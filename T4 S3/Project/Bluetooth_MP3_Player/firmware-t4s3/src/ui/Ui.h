#pragma once

#include <Arduino.h>
#include <vector>
#include <lvgl.h>
#include "../bt/BridgeLink.h"

// =============================================================================
//  Touch-only LVGL interface (450x600 portrait, dark tech theme).
//
//  Screens:
//    splash  ->  devices (scan list, tap to connect)  ->  player (transport)
//
//  The Ui class only builds/updates widgets. It never touches app logic:
//  user actions are reported through the on* callbacks, state arrives through
//  the set*/refresh methods - all invoked from the LVGL task by App.
// =============================================================================

class Ui {
public:
    // ---- callbacks wired by App ----
    void (*onDeviceTapped)(int listIndex) = nullptr;
    void (*onScanAgain)() = nullptr;
    void (*onPlayerBack)() = nullptr;
    void (*onTogglePlay)() = nullptr;
    void (*onNext)() = nullptr;
    void (*onPrev)() = nullptr;
    void (*onSeekMs)(uint32_t ms) = nullptr;
    void (*onVolume)(uint8_t v) = nullptr;

    void begin();

    // ---- screen navigation ----
    void showPlayer();                  // slide left
    void backToDevices();               // slide right

    // ---- state pushed from App (LVGL task context only) ----
    void setBattery(uint8_t pct, bool charging);
    void setBridgeAlive(bool up);
    void setScanStatus(const char *text, bool spinnerOn);
    void refreshDeviceList(const std::vector<BridgeScanItem> &items, int connectingIdx);
    void toast(const char *msg);

    // player state
    void setTrackInfo(const char *title, const char *artist, int index, int count);
    void setProgress(uint32_t elapsedMs, uint32_t durationMs);
    void setPlayState(bool playing);
    void setVolume(uint8_t v);

private:
    void buildStatusBar(lv_obj_t *parent, const char *title, bool withBack);
    void buildBatteryWidget(lv_obj_t *parent, bool forPlayer);
    void buildDevicesScreen();
    void buildPlayerScreen();
    void rebuildDeviceRows(const std::vector<BridgeScanItem> &items);
    void updateBatteryFill();
    void animPulse(lv_obj_t *obj, bool on);

    // LVGL event callbacks (static; g_ui points at the single instance)
    static void cbScanAgain(lv_event_t *e);
    static void cbDeviceTap(lv_event_t *e);
    static void cbBack(lv_event_t *e);
    static void cbToggle(lv_event_t *e);
    static void cbNext(lv_event_t *e);
    static void cbPrev(lv_event_t *e);
    static void cbSeek(lv_event_t *e);
    static void cbSeekStart(lv_event_t *e);
    static void cbSeekLost(lv_event_t *e);
    static void cbVolume(lv_event_t *e);

    // battery widgets, one set per status bar
    lv_obj_t *batFillDev = nullptr, *batPctDev = nullptr, *batBoltDev = nullptr;
    lv_obj_t *batFillPl = nullptr, *batPctPl = nullptr, *batBoltPl = nullptr;
    uint8_t batPct = 100;
    bool batCharging = false;

    // splash
    lv_obj_t *scrSplash = nullptr;

    // devices screen
    lv_obj_t *scrDevices = nullptr;
    lv_obj_t *devBanner = nullptr;
    lv_obj_t *devSpinner = nullptr;
    lv_obj_t *devStatusLabel = nullptr;
    lv_obj_t *devList = nullptr;
    lv_obj_t *devEmpty = nullptr;
    lv_obj_t *devScanBtn = nullptr;

    // player screen
    lv_obj_t *scrPlayer = nullptr;
    lv_obj_t *plIndexLabel = nullptr;
    lv_obj_t *plArt = nullptr;
    lv_obj_t *plArtSymbol = nullptr;
    lv_obj_t *plTitle = nullptr;
    lv_obj_t *plArtist = nullptr;
    lv_obj_t *plElapsed = nullptr;
    lv_obj_t *plTotal = nullptr;
    lv_obj_t *plSlider = nullptr;
    lv_obj_t *plPlayBtn = nullptr;
    lv_obj_t *plPlayIcon = nullptr;
    lv_obj_t *plVolSlider = nullptr;
    lv_obj_t *plVolPct = nullptr;

    // local UI state
    int devCount = 0;
    int connectingIdx = -1;
    bool sliderDragging = false;
    bool pulseRunning = false;
    uint32_t lastDurationMs = 0;
    char lastStatusText[64] = {0};
    bool lastSpinnerOn = false;
};
