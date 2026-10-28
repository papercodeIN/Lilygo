#include "App.h"
#include "config.h"
#include <algorithm>
#include <LilyGo_AMOLED.h>
#include <LV_Helper.h>
#include <SD.h>
#include <esp_sleep.h>

AppClass App;
static LilyGo_Class *g_amoled = nullptr;

// =============================================================================
// boot
// =============================================================================
void AppClass::begin(LilyGo_Class &device) {
    g_amoled = &device;
    bootMs = millis();
    listMutex = xSemaphoreCreateMutex();

    prefs.begin("btmp3", false);
    volume = prefs.getUChar("vol", 80);
    if (volume > 100) volume = 80;

    // ---- SD card library (mount point /fs was created by the LilyGo init) ----
    bool sdOk = scanSdCard();

    // ---- audio + link ----
    player.begin(&tracks);
    BridgeLink::begin();
    BridgeLink::onAliveChanged(bridgeAliveCb);
    BridgeLink::onScanItem(bridgeScanItemCb);
    BridgeLink::onScanDone(bridgeScanDoneCb);
    BridgeLink::onStatus(bridgeStatusCb);
    BridgeLink::onAvrcpKey(bridgeAvrcpCb);

    // ---- UI (all LVGL + Wire calls happen before the LVGL task starts) ----
    ui.begin();
    ui.onDeviceTapped = uiDeviceTappedCb;
    ui.onScanAgain = uiScanAgainCb;
    ui.onPlayerBack = uiPlayerBackCb;
    ui.onTogglePlay = uiTogglePlayCb;
    ui.onNext = uiNextCb;
    ui.onPrev = uiPrevCb;
    ui.onSeekMs = uiSeekCb;
    ui.onVolume = uiVolumeCb;
    ui.setVolume(volume);
    ui.setBattery(100, false);
    ui.setScanStatus("Waiting for bridge...", false);

    power.begin(device);  // SY6970 over Wire - still single-threaded here

#if ENABLE_LOCAL_SPEAKER
    // pseudo-device so the UI can route audio to the local I2S DAC
    BridgeScanItem spk;
    spk.name = LOCAL_SPEAKER_NAME;
    spk.rssi = 0;
    devices.push_back(spk);
    devicesChanged = true;
#endif

    // timers first, then the task that runs them (avoids any init race)
    lv_timer_create(uiPollTrampoline, UI_POLL_PERIOD_MS, nullptr);
    lv_timer_t *t = lv_timer_create(
        [](lv_timer_t *) {
            App.requestScreen(Screen::DEVICES);
        },
        1600, nullptr);
    lv_timer_set_repeat_count(t, 1);

    // LVGL runs in its own task on core 0 from here on.
    xTaskCreatePinnedToCore(lvglTaskTrampoline, "lvgl", UI_TASK_STACK_SIZE,
                            nullptr, UI_TASK_PRIORITY, nullptr, UI_TASK_CORE);

    if (!sdOk) postToast("No SD card or no MP3 files found");
}

// =============================================================================
// SD card scan (root + one level of folders)
// =============================================================================
String AppClass::stripExtension(const String &name) {
    int dot = name.lastIndexOf('.');
    return (dot > 0) ? name.substring(0, dot) : name;
}

bool AppClass::iequalsSuffix(const String &name, const char *suffix) {
    String s(name);
    s.toUpperCase();
    String t(suffix);
    t.toUpperCase();
    return s.endsWith(t);
}

bool AppClass::scanSdCard() {
    tracks.clear();
    File root = SD.open("/");
    if (!root || !root.isDirectory()) {
        log_e("SD mount missing");
        return false;
    }
    std::vector<String> dirs;
    auto addTrack = [&](const String &path, const String &name) {
        if (tracks.size() >= MAX_TRACKS) return;
        if (name.startsWith(".") || name.startsWith("._")) return;
        if (!iequalsSuffix(name, ".mp3")) return;
        TrackItem t;
        t.path = path;
        t.title = stripExtension(name);
        tracks.push_back(t);
    };

    for (File f = root.openNextFile(); f && tracks.size() < MAX_TRACKS; f = root.openNextFile()) {
        String n = f.name();
        if (n.startsWith(".")) continue;
        if (f.isDirectory()) {
            if (n == "System Volume Information") continue;
            dirs.push_back(n);
        } else {
            addTrack(String("/") + n, n);
        }
        f.close();
    }
    for (auto &d : dirs) {
        if (tracks.size() >= MAX_TRACKS) break;
        File dir = SD.open("/" + d);
        if (!dir) continue;
        for (File f = dir.openNextFile(); f && tracks.size() < MAX_TRACKS; f = dir.openNextFile()) {
            String n = f.name();
            if (!f.isDirectory()) addTrack("/" + d + "/" + n, n);
            f.close();
        }
        dir.close();
    }
    root.close();

    std::sort(tracks.begin(), tracks.end(),
              [](const TrackItem &a, const TrackItem &b) { return a.path < b.path; });
    log_i("SD scan: %d tracks", tracks.size());
    return !tracks.empty();
}

// =============================================================================
// FreeRTOS plumbing
// =============================================================================
void AppClass::lvglTaskTrampoline(void *) {
    for (;;) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void AppClass::uiPollTrampoline(lv_timer_t *) { App.uiPoll(); }

// =============================================================================
// LVGL task: periodic refresh of everything the UI shows
// =============================================================================
void AppClass::uiPoll() {
    uint32_t now = millis();

    // battery
    if (now - lastBatteryPollMs >= BATTERY_POLL_MS) {
        lastBatteryPollMs = now;
        power.poll();
        ui.setBattery(power.percent(), power.isCharging());
        if (!power.isCharging() && power.percent() <= LOW_BATTERY_PERCENT && !lowBatteryWarned) {
            lowBatteryWarned = true;
            postToast("Battery low - connect charger");
        }
        if (!power.isCharging() && !power.isPlugged() && power.percent() == 0) {
            esp_sleep_enable_ext0_wakeup((gpio_num_t)0, 0);  // BOOT button wakes
            esp_deep_sleep_start();  // protect the cell
        }
    }

    // pending screen switch
    if (pendingScreen != Screen::NONE) {
        Screen s = pendingScreen;
        pendingScreen = Screen::NONE;
        if (s == Screen::DEVICES) {
            ui.backToDevices();
            ui.setScanStatus(scanning ? "Scanning for audio devices..." : "Tap a device to connect",
                             scanning);
        } else if (s == Screen::PLAYER) {
            ui.showPlayer();
        }
    }

    // bridge presence
    ui.setBridgeAlive(BridgeLink::isAlive());

    // device list
    if (devicesChanged) {
        devicesChanged = false;
        std::vector<BridgeScanItem> copy;
        if (xSemaphoreTake(listMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            copy = devices;
            xSemaphoreGive(listMutex);
        }
        ui.refreshDeviceList(copy, connectingIdx);
    }

    // scan / connect status line
    if (BridgeLink::isAlive()) {
        if (bridgeState == BRP_LINK_CONNECTING) {
            ui.setScanStatus((String("Connecting to ") + connectingName + "...").c_str(), true);
        } else if (scanning) {
            ui.setScanStatus("Scanning for audio devices...", true);
        } else if (bridgeState == BRP_LINK_CONNECTED) {
            ui.setScanStatus("Connected", false);
        } else {
            char buf[48];
            snprintf(buf, sizeof(buf), "%d device(s) - tap to connect", (int)devices.size());
            ui.setScanStatus(buf, false);
        }
    } else {
        ui.setScanStatus("Bridge not detected", false);
    }

    // player widgets
    bool isPlaying = player.isPlaying() && !player.isPaused();
    ui.setPlayState(isPlaying);
    if (player.trackChanged()) {
        TrackItem cur;
        player.currentTrack(cur);
        ui.setTrackInfo(cur.title.c_str(), cur.artist.c_str(),
                        player.currentIndex(), (int)tracks.size());
    }
    ui.setProgress(player.elapsedMs(), player.durationMs());

    // deferred NVS volume save (avoid flash wear while dragging)
    if (lastVolumeChangeMs && now - lastVolumeChangeMs > 2000) {
        lastVolumeChangeMs = 0;
        prefs.putUChar("vol", volume);
    }

    // pending toast
    if (toastPending) {
        toastPending = false;
        ui.toast(toastBuf);
    }

    applyBrightness();
}

void AppClass::applyBrightness() {
    uint32_t inactive = lv_disp_get_inactive_time(NULL);
    uint8_t target = 200;
    if (inactive > OFF_TIMEOUT_MS) target = SCREEN_OFF_BRIGHTNESS;
    else if (inactive > DIM_TIMEOUT_MS) target = DIM_BRIGHTNESS_LEVEL;
    if (target != currentBrightness) {
        currentBrightness = target;
        if (g_amoled) g_amoled->setBrightness(target);
    }
}

// =============================================================================
// loop-task helpers
// =============================================================================
void AppClass::tick() { BridgeLink::poll(); }

void AppClass::postToast(const char *msg) {
    strncpy(toastBuf, msg, sizeof(toastBuf) - 1);
    toastBuf[sizeof(toastBuf) - 1] = 0;
    toastPending = true;
}

void AppClass::requestScreen(Screen s) { pendingScreen = s; }

// =============================================================================
// bridge events (loop task context)
// =============================================================================
void AppClass::bridgeAliveCb(bool up) {
    if (up) {
        BridgeLink::sendScanStart();  // always start from a fresh scan
    } else {
        App.player.stop();
        App.requestScreen(Screen::DEVICES);
        App.postToast("Audio bridge lost");
    }
}

void AppClass::bridgeScanItemCb(const BridgeScanItem &item) {
    if (xSemaphoreTake(App.listMutex, pdMS_TO_TICKS(50)) != pdTRUE) return;
    bool dup = false;
    for (auto &d : App.devices) dup = dup || !memcmp(d.addr, item.addr, 6);
    if (!dup && App.devices.size() < SCAN_DEDUPE_MAX) {
        App.devices.push_back(item);
        App.devicesChanged = true;
    }
    xSemaphoreGive(App.listMutex);
}

void AppClass::bridgeScanDoneCb() { App.scanning = false; }

void AppClass::bridgeStatusCb(uint8_t state, const char *name) {
    App.handleStatus(state, name);
}

void AppClass::handleStatus(uint8_t state, const char *name) {
    bridgeState = state;
    switch (state) {
        case BRP_LINK_CONNECTED: {
            strncpy(connectingName, name, sizeof(connectingName) - 1);
            scanning = false;
            connectingIdx = -1;
            devicesChanged = true;
            player.setOutput(PlayerOutput::Bridge);
            BridgeLink::sendVolume(volume);
            player.playIndex(0);
            requestScreen(Screen::PLAYER);
            break;
        }
        case BRP_LINK_CONNECTING: {
            strncpy(connectingName, name ? name : "", sizeof(connectingName) - 1);
            break;
        }
        case BRP_LINK_DISCONNECTED: {
            player.stop();
            scanning = true;
            connectingIdx = -1;
            devicesChanged = true;
            requestScreen(Screen::DEVICES);
            postToast("Headphones disconnected");
            BridgeLink::sendScanStart();
            break;
        }
        case BRP_LINK_IDLE: {
            scanning = true;
            BridgeLink::sendScanStart();
            break;
        }
        default:
            break;
    }
}

void AppClass::bridgeAvrcpCb(uint8_t key) {
    switch (key) {
        case BRP_AVRCP_PLAY:
            if (App.player.isPaused()) App.player.togglePause();
            break;
        case BRP_AVRCP_PAUSE:
            if (!App.player.isPaused()) App.player.togglePause();
            break;
        case BRP_AVRCP_FORWARD: App.player.next(); break;
        case BRP_AVRCP_BACKWARD: App.player.prev(); break;
        case BRP_AVRCP_VOL_UP:
            App.setVolume(min(100, (int)App.volume + 5), true);
            break;
        case BRP_AVRCP_VOL_DOWN:
            App.setVolume(App.volume > 5 ? App.volume - 5 : 0, true);
            break;
        default: break;
    }
}

// =============================================================================
// UI callbacks (LVGL task)
// =============================================================================
void AppClass::uiDeviceTappedCb(int listIndex) {
    App.connectDevice(listIndex);
}

void AppClass::connectDevice(int listIndex) {
    BridgeScanItem target;
    bool localSpeaker = false;

#if ENABLE_LOCAL_SPEAKER
    if (listIndex == 0 && !App.devices.empty() && App.devices[0].name == LOCAL_SPEAKER_NAME) {
        localSpeaker = true;
    } else
#endif
    {
        if (xSemaphoreTake(App.listMutex, pdMS_TO_TICKS(50)) != pdTRUE) return;
        if (listIndex < 0 || listIndex >= (int)App.devices.size()) {
            xSemaphoreGive(App.listMutex);
            return;
        }
        target = App.devices[listIndex];
        xSemaphoreGive(App.listMutex);
    }

    if (localSpeaker) {
        App.connectingIdx = -1;
        App.player.setOutput(PlayerOutput::Speaker);
        App.player.playIndex(0);
        App.requestScreen(Screen::PLAYER);
        return;
    }

    App.connectingIdx = listIndex;
    App.devicesChanged = true;  // highlight the tapped row
    strncpy(App.connectingName, target.name.c_str(), sizeof(App.connectingName) - 1);
    App.bridgeState = BRP_LINK_CONNECTING;
    BridgeLink::sendConnect(target.addr);
}

void AppClass::uiScanAgainCb() {
    if (!BridgeLink::isAlive()) {
        App.postToast("Bridge dongle not connected");
        return;
    }
    if (xSemaphoreTake(App.listMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        App.devices.clear();
        xSemaphoreGive(App.listMutex);
    }
    App.devicesChanged = true;
    App.scanning = true;
    BridgeLink::sendScanStart();
}

void AppClass::uiPlayerBackCb() {
    App.player.stop();
    BridgeLink::sendDisconnect();
    App.requestScreen(Screen::DEVICES);
}

void AppClass::uiTogglePlayCb() { App.player.togglePause(); }
void AppClass::uiNextCb() { App.player.next(); }
void AppClass::uiPrevCb() { App.player.prev(); }
void AppClass::uiSeekCb(uint32_t ms) { App.player.seekMs(ms); }

void AppClass::uiVolumeCb(uint8_t v) { App.setVolume(v, false); }

void AppClass::setVolume(uint8_t v, bool deferSave) {
    if (v > 100) v = 100;
    volume = v;
    player.setVolume(v);                 // used by the local-speaker path
    if (BridgeLink::isAlive()) BridgeLink::sendVolume(v);  // AVRCP absolute volume
    lastVolumeChangeMs = deferSave ? 0 : millis();
}
