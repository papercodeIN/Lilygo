#include "Ui.h"
#include "Theme.h"
#include "config.h"

using namespace Theme;

static Ui *g_ui = nullptr;  // single instance, set in begin()

// ---- font shorthands (montserrat 12..48 are enabled by the LilyGo lv_conf) ----
#define F12 &lv_font_montserrat_12
#define F14 &lv_font_montserrat_14
#define F16 &lv_font_montserrat_16
#define F18 &lv_font_montserrat_18
#define F20 &lv_font_montserrat_20
#define F28 &lv_font_montserrat_28
#define F32 &lv_font_montserrat_32
#define F48 &lv_font_montserrat_48

static void fmtTime(char *out, size_t n, uint32_t ms) {
    uint32_t s = ms / 1000;
    snprintf(out, n, "%02u:%02u", (unsigned)(s / 60), (unsigned)(s % 60));
}

static void styleSlider(lv_obj_t *s, lv_coord_t w, lv_coord_t knobD, lv_coord_t trackH) {
    lv_obj_set_size(s, w, knobD);
    lv_obj_set_style_pad_top(s, (knobD - trackH) / 2, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(s, (knobD - trackH) / 2, LV_PART_MAIN);
    lv_obj_set_style_pad_left(s, knobD / 2, LV_PART_MAIN);
    lv_obj_set_style_pad_right(s, knobD / 2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s, C(TrackBar), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(s, trackH / 2, LV_PART_MAIN);
    lv_obj_set_style_border_width(s, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s, C(Accent), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(s, trackH / 2, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s, C(0xFFFFFF), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_radius(s, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_set_style_border_width(s, 0, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(s, 0, LV_PART_KNOB);
}

// ---- static event callbacks ------------------------------------------------
void Ui::cbScanAgain(lv_event_t *e) {
    if (g_ui && g_ui->onScanAgain) g_ui->onScanAgain();
}
void Ui::cbDeviceTap(lv_event_t *e) {
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (g_ui && g_ui->onDeviceTapped) g_ui->onDeviceTapped(idx);
}
void Ui::cbBack(lv_event_t *e) {
    if (g_ui && g_ui->onPlayerBack) g_ui->onPlayerBack();
}
void Ui::cbToggle(lv_event_t *e) {
    if (g_ui && g_ui->onTogglePlay) g_ui->onTogglePlay();
}
void Ui::cbNext(lv_event_t *e) {
    if (g_ui && g_ui->onNext) g_ui->onNext();
}
void Ui::cbPrev(lv_event_t *e) {
    if (g_ui && g_ui->onPrev) g_ui->onPrev();
}
void Ui::cbSeek(lv_event_t *e) {
    lv_obj_t *s = (lv_obj_t *)lv_event_get_target(e);
    if (!g_ui) return;
    g_ui->sliderDragging = false;  // resume progress updates even if seek was a no-op
    if (g_ui->onSeekMs && g_ui->lastDurationMs > 0) {
        uint32_t frac = (uint32_t)lv_slider_get_value(s);
        g_ui->onSeekMs((uint32_t)((uint64_t)frac * g_ui->lastDurationMs / 1000));
    }
}
void Ui::cbSeekStart(lv_event_t *e) {
    if (g_ui) g_ui->sliderDragging = true;
}
void Ui::cbSeekLost(lv_event_t *e) {
    if (g_ui) g_ui->sliderDragging = false;  // drag cancelled (finger slid off)
}
void Ui::cbVolume(lv_event_t *e) {
    lv_obj_t *s = (lv_obj_t *)lv_event_get_target(e);
    int v = lv_slider_get_value(s);
    if (g_ui) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", v);
        lv_label_set_text(g_ui->plVolPct, buf);
        if (g_ui->onVolume) g_ui->onVolume((uint8_t)v);
    }
}

// -----------------------------------------------------------------------------
// build
// -----------------------------------------------------------------------------
void Ui::begin() {
    g_ui = this;

    scrSplash = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scrSplash, C(Bg), 0);
    lv_obj_t *logo = label(scrSplash, LV_SYMBOL_BLUETOOTH, F48, Accent);
    lv_obj_center(logo);
    lv_obj_t *t1 = label(scrSplash, "T4-S3  MP3", F28, TextMain);
    lv_obj_align(t1, LV_ALIGN_CENTER, 0, 62);
    lv_obj_t *t2 = label(scrSplash, "BLUETOOTH  PLAYER", F12, TextSub);
    lv_obj_align(t2, LV_ALIGN_CENTER, 0, 100);
    lv_obj_set_style_text_letter_space(t2, 4, 0);

    buildDevicesScreen();
    buildPlayerScreen();
    lv_scr_load(scrSplash);
}

// -----------------------------------------------------------------------------
// status bar + battery widget
// -----------------------------------------------------------------------------
void Ui::buildStatusBar(lv_obj_t *parent, const char *title, bool withBack) {
    lv_obj_t *t = label(parent, title, F12, TextSub);
    lv_obj_set_style_text_letter_space(t, 3, 0);
    lv_obj_align(t, LV_ALIGN_TOP_LEFT, withBack ? 56 : 16, 14);

    if (withBack) {
        lv_obj_t *back = circleBtn(parent, 30, Surface, LV_SYMBOL_LEFT, F14, TextMain);
        lv_obj_align(back, LV_ALIGN_TOP_LEFT, 12, 5);
        lv_obj_add_event_cb(back, cbBack, LV_EVENT_CLICKED, nullptr);
    }
    lv_obj_t *line = panel(parent, Stroke, 0, 0);
    lv_obj_set_size(line, 450, 1);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 40);
}

void Ui::buildBatteryWidget(lv_obj_t *parent, bool forPlayer) {
    lv_obj_t *host = lv_obj_create(parent);
    lv_obj_remove_style_all(host);
    lv_obj_set_size(host, 96, 20);
    lv_obj_align(host, LV_ALIGN_TOP_RIGHT, -12, 10);

    lv_obj_t *bolt = label(host, LV_SYMBOL_CHARGE, F12, Good);
    lv_obj_align(bolt, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_flag(bolt, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *pill = panel(host, Bg, Stroke, 4);
    lv_obj_set_size(pill, 30, 16);
    lv_obj_align(pill, LV_ALIGN_LEFT_MID, 18, 0);
    lv_obj_t *fill = panel(pill, Good, 0, 2);
    lv_obj_set_size(fill, 24, 10);
    lv_obj_align(fill, LV_ALIGN_LEFT_MID, 2, 0);
    lv_obj_t *cap = panel(host, Stroke, 0, 1);
    lv_obj_set_size(cap, 3, 8);
    lv_obj_align(cap, LV_ALIGN_LEFT_MID, 49, 0);

    lv_obj_t *pct = label(host, "--%", F12, TextSub);
    lv_obj_align(pct, LV_ALIGN_RIGHT_MID, 0, 0);

    if (forPlayer) {
        batBoltPl = bolt; batFillPl = fill; batPctPl = pct;
    } else {
        batBoltDev = bolt; batFillDev = fill; batPctDev = pct;
    }
}

void Ui::updateBatteryFill() {
    uint32_t col = batCharging ? Good : (batPct > 25 ? TextMain : (batPct > 12 ? Warn : Danger));
    lv_coord_t w = 2 + (lv_coord_t)((24 * batPct) / 100);
    char buf[8];
    snprintf(buf, sizeof(buf), "%u%%", batPct);
    lv_obj_t *fills[] = {batFillDev, batFillPl};
    lv_obj_t *pcts[] = {batPctDev, batPctPl};
    lv_obj_t *bolts[] = {batBoltDev, batBoltPl};
    for (int i = 0; i < 2; i++) {
        if (fills[i]) {
            lv_obj_set_size(fills[i], w, 10);
            lv_obj_set_style_bg_color(fills[i], C(col), 0);
        }
        if (pcts[i]) lv_label_set_text(pcts[i], buf);
        if (bolts[i]) {
            if (batCharging) lv_obj_clear_flag(bolts[i], LV_OBJ_FLAG_HIDDEN);
            else lv_obj_add_flag(bolts[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

// -----------------------------------------------------------------------------
// devices screen
// -----------------------------------------------------------------------------
void Ui::buildDevicesScreen() {
    scrDevices = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scrDevices, C(Bg), 0);
    lv_obj_clear_flag(scrDevices, LV_OBJ_FLAG_SCROLLABLE);
    buildStatusBar(scrDevices, "BLUETOOTH  AUDIO", false);
    buildBatteryWidget(scrDevices, false);

    // header card
    lv_obj_t *head = panel(scrDevices, Surface, Stroke, 16);
    lv_obj_set_size(head, 418, 56);
    lv_obj_align(head, LV_ALIGN_TOP_MID, 0, 52);
    lv_obj_t *ic = circleBtn(head, 36, AccentDim, LV_SYMBOL_BLUETOOTH, F16, Accent);
    lv_obj_align(ic, LV_ALIGN_LEFT_MID, 10, 0);
    devStatusLabel = label(head, "Starting...", F16, TextMain);
    lv_obj_align(devStatusLabel, LV_ALIGN_LEFT_MID, 58, 0);
    devSpinner = lv_spinner_create(head, 900, 60);
    lv_obj_set_size(devSpinner, 26, 26);
    lv_obj_set_style_arc_color(devSpinner, C(Accent), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(devSpinner, C(TrackBar), LV_PART_MAIN);
    lv_obj_set_style_pad_all(devSpinner, 2, 0);
    lv_obj_align(devSpinner, LV_ALIGN_RIGHT_MID, -14, 0);

    // bridge-offline banner
    devBanner = panel(scrDevices, 0x2A1E08, Warn, 12);
    lv_obj_set_size(devBanner, 418, 40);
    lv_obj_align(devBanner, LV_ALIGN_TOP_MID, 0, 116);
    lv_obj_t *bi = label(devBanner, LV_SYMBOL_WARNING, F16, Warn);
    lv_obj_align(bi, LV_ALIGN_LEFT_MID, 14, 0);
    lv_obj_t *bt = label(devBanner, "Bridge dongle not connected", F12, Warn);
    lv_obj_align(bt, LV_ALIGN_LEFT_MID, 38, 0);
    lv_obj_add_flag(devBanner, LV_OBJ_FLAG_HIDDEN);

    // device list (touch-scrollable)
    devList = lv_obj_create(scrDevices);
    lv_obj_remove_style_all(devList);
    lv_obj_set_size(devList, 450, 360);
    lv_obj_align(devList, LV_ALIGN_TOP_MID, 0, 166);
    lv_obj_set_flex_flow(devList, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_left(devList, 16, 0);
    lv_obj_set_style_pad_right(devList, 16, 0);
    lv_obj_set_style_pad_top(devList, 4, 0);
    lv_obj_set_style_pad_row(devList, 8, 0);
    lv_obj_set_scroll_dir(devList, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(devList, LV_SCROLLBAR_MODE_OFF);

    // empty state
    devEmpty = lv_obj_create(scrDevices);
    lv_obj_remove_style_all(devEmpty);
    lv_obj_set_size(devEmpty, 418, 200);
    lv_obj_align(devEmpty, LV_ALIGN_TOP_MID, 0, 240);
    lv_obj_t *ei = circleBtn(devEmpty, 72, Surface, LV_SYMBOL_BLUETOOTH, F32, TextSub);
    lv_obj_align(ei, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_t *et = label(devEmpty, "No devices found", F20, TextMain);
    lv_obj_align(et, LV_ALIGN_TOP_MID, 0, 92);
    lv_obj_t *eh = label(devEmpty,
                         "Power the bridge dongle and put your\nheadphones or speaker in pairing mode",
                         F12, TextSub);
    lv_obj_set_style_text_align(eh, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(eh, LV_ALIGN_TOP_MID, 0, 124);
    lv_obj_add_flag(devEmpty, LV_OBJ_FLAG_HIDDEN);

    // scan-again footer
    devScanBtn = lv_btn_create(scrDevices);
    lv_obj_remove_style_all(devScanBtn);
    lv_obj_set_size(devScanBtn, 418, 54);
    lv_obj_align(devScanBtn, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_obj_set_style_radius(devScanBtn, 27, 0);
    lv_obj_set_style_bg_color(devScanBtn, C(Surface), 0);
    lv_obj_set_style_bg_opa(devScanBtn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(devScanBtn, C(Stroke), 0);
    lv_obj_set_style_border_width(devScanBtn, 1, 0);
    lv_obj_set_style_bg_color(devScanBtn, C(SurfaceHi), LV_STATE_PRESSED);
    lv_obj_t *sbi = lv_label_create(devScanBtn);
    lv_label_set_text_fmt(sbi, "%s  SCAN AGAIN", LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(sbi, F16, 0);
    lv_obj_set_style_text_color(sbi, C(Accent), 0);
    lv_obj_center(sbi);
    lv_obj_add_event_cb(devScanBtn, cbScanAgain, LV_EVENT_CLICKED, nullptr);
}

void Ui::rebuildDeviceRows(const std::vector<BridgeScanItem> &items) {
    lv_obj_clean(devList);
    devCount = (int)items.size();
    for (int i = 0; i < devCount && i < SCAN_DEDUPE_MAX; i++) {
        lv_obj_t *row = panel(devList, Surface, Stroke, 14);
        lv_obj_set_size(row, 418, 66);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_color(row, C(SurfaceHi), LV_STATE_PRESSED);
        if (i == connectingIdx) {
            lv_obj_set_style_border_color(row, C(Accent), 0);
            lv_obj_set_style_border_width(row, 2, 0);
        }

        lv_obj_t *ic = circleBtn(row, 38, AccentDim, LV_SYMBOL_BLUETOOTH, F16, Accent);
        lv_obj_align(ic, LV_ALIGN_LEFT_MID, 10, 0);

        lv_obj_t *nm = label(row, items[i].name.c_str(), F18, TextMain);
        lv_obj_align(nm, LV_ALIGN_LEFT_MID, 60, 0);
        lv_obj_set_width(nm, 250);
        lv_label_set_long_mode(nm, LV_LABEL_LONG_DOT);

        uint8_t bars = 0;
        int rssi = items[i].rssi;
        if (rssi > -55) bars = 4;
        else if (rssi > -65) bars = 3;
        else if (rssi > -75) bars = 2;
        else if (rssi > -88) bars = 1;
        rssiBars(row, 418 - 52, 33, bars, Accent, TrackBar);

        lv_obj_add_event_cb(row, cbDeviceTap, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
    if (devCount == 0) lv_obj_clear_flag(devEmpty, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(devEmpty, LV_OBJ_FLAG_HIDDEN);
}

// -----------------------------------------------------------------------------
// player screen
// -----------------------------------------------------------------------------
void Ui::buildPlayerScreen() {
    scrPlayer = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scrPlayer, C(Bg), 0);
    lv_obj_clear_flag(scrPlayer, LV_OBJ_FLAG_SCROLLABLE);
    buildStatusBar(scrPlayer, "NOW  PLAYING", true);
    buildBatteryWidget(scrPlayer, true);

    plIndexLabel = label(scrPlayer, "TRACK  0 / 0", F12, TextSub);
    lv_obj_set_style_text_letter_space(plIndexLabel, 2, 0);
    lv_obj_align(plIndexLabel, LV_ALIGN_TOP_MID, 0, 56);

    // album-art card with glow ring
    plArt = panel(scrPlayer, Surface, Stroke, 26);
    lv_obj_set_size(plArt, 236, 236);
    lv_obj_align(plArt, LV_ALIGN_TOP_MID, 0, 84);
    lv_obj_set_style_shadow_width(plArt, 60, 0);
    lv_obj_set_style_shadow_color(plArt, C(Accent), 0);
    lv_obj_set_style_shadow_opa(plArt, LV_OPA_20, 0);
    plArtSymbol = label(plArt, LV_SYMBOL_AUDIO, F48, Accent);
    lv_obj_center(plArtSymbol);

    plTitle = label(scrPlayer, "-", F28, TextMain);
    lv_obj_align(plTitle, LV_ALIGN_TOP_MID, 0, 340);
    lv_obj_set_width(plTitle, 410);
    lv_label_set_long_mode(plTitle, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(plTitle, LV_TEXT_ALIGN_CENTER, 0);

    plArtist = label(scrPlayer, "-", F16, TextSub);
    lv_obj_align(plArtist, LV_ALIGN_TOP_MID, 0, 382);
    lv_obj_set_width(plArtist, 410);
    lv_label_set_long_mode(plArtist, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(plArtist, LV_TEXT_ALIGN_CENTER, 0);

    plElapsed = label(scrPlayer, "00:00", F14, TextSub);
    lv_obj_align(plElapsed, LV_ALIGN_TOP_LEFT, 30, 424);
    plTotal = label(scrPlayer, "00:00", F14, TextSub);
    lv_obj_align(plTotal, LV_ALIGN_TOP_RIGHT, -30, 424);

    plSlider = lv_slider_create(scrPlayer);
    styleSlider(plSlider, 390, 18, 8);
    lv_slider_set_range(plSlider, 0, 1000);
    lv_slider_set_value(plSlider, 0, LV_ANIM_OFF);
    lv_obj_align(plSlider, LV_ALIGN_TOP_MID, 0, 448);
    lv_obj_add_event_cb(plSlider, cbSeekStart, LV_EVENT_PRESSING, nullptr);
    lv_obj_add_event_cb(plSlider, cbSeek, LV_EVENT_RELEASED, nullptr);
    lv_obj_add_event_cb(plSlider, cbSeekLost, LV_EVENT_PRESS_LOST, nullptr);

    // transport
    plPlayBtn = circleBtn(scrPlayer, 84, Accent, LV_SYMBOL_PLAY, F32, 0xFFFFFF);
    lv_obj_align(plPlayBtn, LV_ALIGN_TOP_MID, 0, 486);
    lv_obj_set_style_shadow_width(plPlayBtn, 40, 0);
    lv_obj_set_style_shadow_opa(plPlayBtn, LV_OPA_30, 0);
    lv_obj_add_event_cb(plPlayBtn, cbToggle, LV_EVENT_CLICKED, nullptr);
    plPlayIcon = lv_obj_get_child(plPlayBtn, 0);

    lv_obj_t *prevBtn = circleBtn(scrPlayer, 58, Surface, LV_SYMBOL_PREV, F20, TextMain);
    lv_obj_align(prevBtn, LV_ALIGN_TOP_MID, -110, 499);
    lv_obj_add_event_cb(prevBtn, cbPrev, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *nextBtn = circleBtn(scrPlayer, 58, Surface, LV_SYMBOL_NEXT, F20, TextMain);
    lv_obj_align(nextBtn, LV_ALIGN_TOP_MID, 110, 499);
    lv_obj_add_event_cb(nextBtn, cbNext, LV_EVENT_CLICKED, nullptr);

    // volume row
    lv_obj_t *volIcon = label(scrPlayer, LV_SYMBOL_VOLUME_MAX, F16, TextSub);
    lv_obj_align(volIcon, LV_ALIGN_BOTTOM_LEFT, 30, -22);
    plVolSlider = lv_slider_create(scrPlayer);
    styleSlider(plVolSlider, 296, 14, 6);
    lv_slider_set_range(plVolSlider, 0, 100);
    lv_slider_set_value(plVolSlider, 80, LV_ANIM_OFF);
    lv_obj_align(plVolSlider, LV_ALIGN_BOTTOM_LEFT, 66, -20);
    lv_obj_add_event_cb(plVolSlider, cbVolume, LV_EVENT_VALUE_CHANGED, nullptr);
    plVolPct = label(scrPlayer, "80", F14, TextSub);
    lv_obj_align(plVolPct, LV_ALIGN_BOTTOM_RIGHT, -30, -22);
}

// -----------------------------------------------------------------------------
// navigation
// -----------------------------------------------------------------------------
void Ui::showPlayer() { lv_scr_load_anim(scrPlayer, LV_SCR_LOAD_ANIM_MOVE_LEFT, 260, 0, false); }
void Ui::backToDevices() { lv_scr_load_anim(scrDevices, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 260, 0, false); }

// -----------------------------------------------------------------------------
// state setters
// -----------------------------------------------------------------------------
void Ui::setBattery(uint8_t pct, bool charging) {
    batPct = pct;
    batCharging = charging;
    updateBatteryFill();
}

void Ui::setBridgeAlive(bool up) {
    if (up) lv_obj_add_flag(devBanner, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(devBanner, LV_OBJ_FLAG_HIDDEN);
}

void Ui::setScanStatus(const char *text, bool spinnerOn) {
    // App polls every 100 ms with unchanged text - skip redundant LVGL work
    if (!text) text = "";
    if (strcmp(text, lastStatusText) != 0) {
        strncpy(lastStatusText, text, sizeof(lastStatusText) - 1);
        lastStatusText[sizeof(lastStatusText) - 1] = 0;
        lv_label_set_text(devStatusLabel, lastStatusText);
    }
    if (spinnerOn != lastSpinnerOn) {
        lastSpinnerOn = spinnerOn;
        if (spinnerOn) lv_obj_clear_flag(devSpinner, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(devSpinner, LV_OBJ_FLAG_HIDDEN);
    }
}

void Ui::refreshDeviceList(const std::vector<BridgeScanItem> &items, int connecting) {
    connectingIdx = connecting;
    rebuildDeviceRows(items);
}

// -----------------------------------------------------------------------------
// toast (top layer, auto-hide)
// -----------------------------------------------------------------------------
void Ui::toast(const char *msg) {
    static lv_obj_t *toastBox = nullptr;
    if (!toastBox) {
        toastBox = panel(lv_layer_top(), SurfaceHi, Stroke, 14);
        lv_obj_set_size(toastBox, 380, 56);
        lv_obj_align(toastBox, LV_ALIGN_TOP_MID, 0, 60);
        lv_obj_set_style_shadow_width(toastBox, 40, 0);
        lv_obj_set_style_shadow_opa(toastBox, LV_OPA_40, 0);
        lv_obj_add_flag(toastBox, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_opa(toastBox, LV_OPA_90, 0);
    }
    // single label child
    while (lv_obj_get_child_cnt(toastBox) > 0) lv_obj_del(lv_obj_get_child(toastBox, 0));
    lv_obj_t *l = label(toastBox, msg, F14, TextMain);
    lv_obj_set_width(l, 350);
    lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(l);
    lv_obj_clear_flag(toastBox, LV_OBJ_FLAG_HIDDEN);

    static lv_timer_t *hideTimer = nullptr;
    if (hideTimer) lv_timer_del(hideTimer);
    hideTimer = lv_timer_create(
        [](lv_timer_t *t) {
            lv_obj_t *box = (lv_obj_t *)t->user_data;
            lv_obj_add_flag(box, LV_OBJ_FLAG_HIDDEN);
        },
        2600, toastBox);
    lv_timer_set_repeat_count(hideTimer, 1);
}

// -----------------------------------------------------------------------------
// player state
// -----------------------------------------------------------------------------
void Ui::setTrackInfo(const char *title, const char *artist, int index, int count) {
    lv_label_set_text(plTitle, title && title[0] ? title : "Unknown title");
    lv_label_set_text(plArtist, artist && artist[0] ? artist : " ");
    char buf[32];
    snprintf(buf, sizeof(buf), "TRACK  %d / %d", index + 1, count);
    lv_label_set_text(plIndexLabel, buf);
}

void Ui::setProgress(uint32_t elapsedMs, uint32_t durationMs) {
    lastDurationMs = durationMs;
    char buf[8];
    fmtTime(buf, sizeof(buf), elapsedMs);
    lv_label_set_text(plElapsed, buf);
    if (durationMs) {
        fmtTime(buf, sizeof(buf), durationMs);
        lv_label_set_text(plTotal, buf);
    } else {
        lv_label_set_text(plTotal, "--:--");
    }
    if (!sliderDragging && durationMs) {
        uint32_t frac = (uint32_t)((uint64_t)elapsedMs * 1000 / durationMs);
        if (frac > 1000) frac = 1000;
        lv_slider_set_value(plSlider, (int32_t)frac, LV_ANIM_OFF);
    } else if (!sliderDragging) {
        lv_slider_set_value(plSlider, 0, LV_ANIM_OFF);
    }
}

void Ui::setPlayState(bool isPlaying) {
    if (plPlayIcon) lv_label_set_text(plPlayIcon, isPlaying ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    animPulse(plArtSymbol, isPlaying);
}

// Properly typed opacity animation callback. Casting lv_obj_set_style_opa
// directly to lv_anim_exec_xcb_t leaves the style-selector argument undefined.
static void anim_opa_cb(void *var, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, 0);
}

void Ui::animPulse(lv_obj_t *obj, bool on) {
    if (on && !pulseRunning) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, obj);
        lv_anim_set_exec_cb(&a, anim_opa_cb);
        lv_anim_set_values(&a, LV_OPA_40, LV_OPA_COVER);
        lv_anim_set_time(&a, 900);
        lv_anim_set_playback_time(&a, 900);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&a);
        pulseRunning = true;
    } else if (!on && pulseRunning) {
        lv_anim_del(obj, anim_opa_cb);
        lv_obj_set_style_opa(obj, LV_OPA_COVER, 0);
        pulseRunning = false;
    }
}

void Ui::setVolume(uint8_t v) {
    lv_slider_set_value(plVolSlider, v, LV_ANIM_OFF);
    char buf[8];
    snprintf(buf, sizeof(buf), "%u", v);
    lv_label_set_text(plVolPct, buf);
}
