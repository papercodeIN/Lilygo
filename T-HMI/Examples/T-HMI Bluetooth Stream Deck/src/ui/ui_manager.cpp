/**
 * @file ui_manager.cpp
 * Deck UI: status header with page dots, BLE connection line, action feedback
 * line and a swipeable tileview of button pages. Swipe left/right (or tap the
 * header arrows) to move between pages; tap a button to send its action.
 */

#include "ui_manager.h"

#include <Arduino.h>

#include <cstddef>
#include <cstdint>

#include "backlight.h"
#include "config.h"
#include "debug_log.h"
#include "../ble_hid_manager.h"
#include "../hid_task.h"
#include "theme.h"

namespace ui {

namespace {

constexpr lv_coord_t HEADER_HEIGHT = 44;
constexpr lv_coord_t STATUS_HEIGHT = 20;
constexpr lv_coord_t GRID_GAP = 8;
constexpr size_t GRID_COLUMNS_COUNT = 3;
constexpr size_t GRID_ROWS_COUNT = 2;
constexpr lv_coord_t GRID_COLUMNS[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
constexpr lv_coord_t GRID_ROWS[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

static_assert(PAGE_COUNT > 0, "config.h must define at least one page");
static_assert(MAX_BUTTONS_PER_PAGE <= GRID_COLUMNS_COUNT * GRID_ROWS_COUNT,
              "Grid cannot hold all configured buttons");

lv_obj_t *title_label;
lv_obj_t *ble_label;
lv_obj_t *action_label;
lv_obj_t *page_dots[PAGE_COUNT] = {};
lv_obj_t *tiles[PAGE_COUNT] = {};
lv_obj_t *tileview;
size_t active_page = 0;
bool rendered_ble_connected = false;
uint32_t last_activity = 0;
bool dimmed = false;

// Thread-safe one-shot feedback slot: HID task (or any task) writes it, the
// LVGL loop drains it. Pointers are config-owned static strings.
portMUX_TYPE feedback_mux = portMUX_INITIALIZER_UNLOCKED;
volatile const char *pending_feedback_text = nullptr;
volatile uint8_t pending_feedback_kind = 0;

lv_color_t feedback_color(Feedback kind) {
    switch (kind) {
    case Feedback::OK:
        return theme::COL_ACCENT;
    case Feedback::WARNING:
        return theme::COL_WARNING;
    case Feedback::ERROR:
    default:
        return theme::COL_ERROR;
    }
}

void set_action_label(const char *text, const lv_color_t &color) {
    lv_label_set_text(action_label, text);
    lv_obj_set_style_text_color(action_label, color, 0);
}

void set_dimmed(bool value) {
    if (dimmed == value) return;
    dimmed = value;
    backlight::set_dimmed(value);
}

void set_active_page(size_t index) {
    if (index >= PAGE_COUNT) return;
    active_page = index;
    lv_label_set_text(title_label, PAGES[index].title);
    for (size_t i = 0; i < PAGE_COUNT; ++i) {
        if (page_dots[i] == nullptr) continue;
        lv_obj_set_style_bg_color(page_dots[i],
                                  i == index ? lv_color_hex(PAGES[i].accent) : theme::COL_BORDER, 0);
    }
    Serial.printf("[UI] page -> %s (%u/%u)\n", PAGES[index].title,
                  static_cast<unsigned>(index + 1), static_cast<unsigned>(PAGE_COUNT));
}

// The tileview emits VALUE_CHANGED after every snap (swipe included) with the
// settled tile tracked in tile_act — the canonical page-change signal.
void tileview_changed_event(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) return;
    const lv_obj_t *act = lv_tileview_get_tile_act(tileview);
    for (size_t i = 0; i < PAGE_COUNT; ++i) {
        if (tiles[i] == act) {
            if (i != active_page) {
                set_active_page(i);
                notify_activity();
            }
            return;
        }
    }
    Serial.println("[UI] VALUE_CHANGED: no matching tile");
}

void step_page(int delta) {
    const int count = static_cast<int>(PAGE_COUNT);
    if (count <= 1) return;
    const int next = ((static_cast<int>(active_page) + delta) % count + count) % count;
    Serial.printf("[UI] arrow %s -> page %d\n", delta < 0 ? "prev" : "next", next + 1);
    lv_obj_set_tile_id(tileview, static_cast<uint32_t>(next), 0, LV_ANIM_ON);
    set_active_page(static_cast<size_t>(next));
    notify_activity();
}

void previous_page_event(lv_event_t *event) {
    if (lv_event_get_code(event) == LV_EVENT_PRESSED) step_page(-1);
}

void next_page_event(lv_event_t *event) {
    if (lv_event_get_code(event) == LV_EVENT_PRESSED) step_page(1);
}

void button_event(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    auto *definition = static_cast<const ButtonDef *>(lv_event_get_user_data(event));
    if (definition == nullptr) return;
    Serial.printf("[UI] tap: %s\n", definition->label);
    notify_activity();
    hid_task::request(definition);
}

void feedback_timer(lv_timer_t *) {
    portENTER_CRITICAL(&feedback_mux);
    const char *text = const_cast<const char *>(pending_feedback_text);
    const uint8_t kind = pending_feedback_kind;
    pending_feedback_text = nullptr;
    portEXIT_CRITICAL(&feedback_mux);
    if (text != nullptr && action_label != nullptr) {
        set_action_label(text, feedback_color(static_cast<Feedback>(kind)));
    }
}

void ble_poll_timer(lv_timer_t *) {
    const bool connected = BLEHID.connected();
    if (connected != rendered_ble_connected) {
        rendered_ble_connected = connected;
        if (ble_label != nullptr) {
            lv_label_set_text(ble_label, connected ? "BT: Connected" : "BT: Pairing");
            lv_obj_set_style_text_color(ble_label, connected ? theme::COL_ACCENT : theme::COL_WARNING, 0);
        }
        // Surface the transition on the action line too.
        set_action_label(connected ? "Ready" : "Not connected",
                         connected ? theme::COL_ACCENT : theme::COL_ERROR);
        Serial.printf("[BLE] host %s\n", connected ? "connected" : "disconnected");
    }
}

void idle_timer(lv_timer_t *) {
    if (static_cast<uint32_t>(lv_tick_get() - last_activity) >= IDLE_DIM_TIMEOUT_MS) set_dimmed(true);
}

void first_refresh_timer(lv_timer_t *timer) {
    lv_obj_invalidate(lv_scr_act());
    lv_timer_del(timer);
}

} // namespace

void show_splash(uint32_t duration_ms, const char *boot_notice) {
    lv_obj_t *deck = lv_scr_act();
    lv_obj_t *splash = lv_obj_create(nullptr);
    lv_obj_remove_style_all(splash);
    lv_obj_set_style_bg_color(splash, theme::COL_BG, 0);
    lv_obj_set_style_bg_opa(splash, LV_OPA_COVER, 0);

    lv_obj_t *brand = lv_label_create(splash);
    lv_label_set_text(brand, "FUSION AUTOMATE");
    lv_obj_set_style_text_color(brand, theme::COL_ACCENT, 0);
    lv_obj_set_style_text_font(brand, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_letter_space(brand, 2, 0);
    lv_obj_align(brand, LV_ALIGN_CENTER, 0, -26);

    lv_obj_t *line = lv_obj_create(splash);
    lv_obj_remove_style_all(line);
    lv_obj_set_size(line, 64, 3);
    lv_obj_set_style_bg_color(line, theme::COL_ACCENT, 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_align(line, LV_ALIGN_CENTER, 0, 2);

    lv_obj_t *tagline = lv_label_create(splash);
    lv_label_set_text(tagline, "BLUETOOTH STREAM DECK");
    lv_obj_set_style_text_color(tagline, theme::COL_MUTED, 0);
    lv_obj_set_style_text_font(tagline, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_letter_space(tagline, 1, 0);
    lv_obj_align(tagline, LV_ALIGN_CENTER, 0, 20);

    char pair_hint[64];
    snprintf(pair_hint, sizeof(pair_hint), "Pair as \"%s\"", BLE_DEVICE_NAME);
    lv_obj_t *hint = lv_label_create(splash);
    lv_label_set_text(hint, pair_hint);
    lv_obj_set_style_text_color(hint, theme::COL_MUTED, 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_align(hint, LV_ALIGN_CENTER, 0, 42);

    // Shown when the device did not boot from a clean power-on (panic,
    // watchdog, brownout, ...) so a reboot loop can be diagnosed from a photo.
    if (boot_notice != nullptr && boot_notice[0] != '\0') {
        lv_obj_t *notice = lv_label_create(splash);
        lv_label_set_text(notice, boot_notice);
        lv_obj_set_style_text_color(notice, theme::COL_WARNING, 0);
        lv_obj_set_style_text_font(notice, &lv_font_montserrat_14, 0);
        lv_obj_align(notice, LV_ALIGN_BOTTOM_MID, 0, -8);
    }

    lv_scr_load(splash);
    lv_timer_handler();
    delay(300);
    backlight::set_dimmed(false);

    const uint32_t start = lv_tick_get();
    while (static_cast<uint32_t>(lv_tick_get() - start) < duration_ms) {
        lv_timer_handler();
        delay(5);
    }

    lv_scr_load(deck);
    lv_obj_del(splash);
    notify_activity(); // start the idle timeout after the splash, not before it
}

void notify_activity() {
    last_activity = lv_tick_get();
    set_dimmed(false);
}

void post_feedback(const char *text, Feedback kind) {
    if (text == nullptr) return;
    portENTER_CRITICAL(&feedback_mux);
    pending_feedback_text = text;
    pending_feedback_kind = static_cast<uint8_t>(kind);
    portEXIT_CRITICAL(&feedback_mux);
}

void build() {
    lv_obj_t *screen = lv_scr_act();
    theme::init();
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    // ---- Header: arrows, title, page dots ----
    lv_obj_t *header = lv_obj_create(screen);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, LV_PCT(100), HEADER_HEIGHT);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, theme::COL_HEADER, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_border_color(header, theme::COL_BORDER, 0);
    lv_obj_update_layout(header);

    title_label = lv_label_create(header);
    lv_obj_set_style_text_color(title_label, theme::COL_TEXT, 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_letter_space(title_label, 1, 0);
    lv_obj_set_width(title_label, 180);
    lv_label_set_long_mode(title_label, LV_LABEL_LONG_DOT);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 3);

    // One dot per page, centered under the title.
    constexpr lv_coord_t DOT_SIZE = 6;
    constexpr lv_coord_t DOT_GAP = 5;
    const lv_coord_t dots_width =
        static_cast<lv_coord_t>(PAGE_COUNT) * DOT_SIZE + (static_cast<lv_coord_t>(PAGE_COUNT) - 1) * DOT_GAP;
    for (size_t i = 0; i < PAGE_COUNT; ++i) {
        lv_obj_t *dot = lv_obj_create(header);
        lv_obj_remove_style_all(dot);
        lv_obj_set_size(dot, DOT_SIZE, DOT_SIZE);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, theme::COL_BORDER, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_align(dot, LV_ALIGN_BOTTOM_MID,
                     static_cast<lv_coord_t>(i) * (DOT_SIZE + DOT_GAP) - dots_width / 2, -4);
        page_dots[i] = dot;
    }

    lv_obj_t *prev_button = theme::make_header_button(header, LV_SYMBOL_LEFT);
    lv_obj_align(prev_button, LV_ALIGN_LEFT_MID, 6, 0);
    lv_obj_set_ext_click_area(prev_button, 20);
    lv_obj_add_event_cb(prev_button, previous_page_event, LV_EVENT_PRESSED, nullptr);

    lv_obj_t *next_button = theme::make_header_button(header, LV_SYMBOL_RIGHT);
    lv_obj_align(next_button, LV_ALIGN_RIGHT_MID, -6, 0);
    lv_obj_set_ext_click_area(next_button, 20);
    lv_obj_add_event_cb(next_button, next_page_event, LV_EVENT_PRESSED, nullptr);

    // ---- Status bar: BLE state left, action feedback right ----
    lv_obj_t *status_bar = lv_obj_create(screen);
    lv_obj_remove_style_all(status_bar);
    lv_obj_set_size(status_bar, LV_PCT(100), STATUS_HEIGHT);
    lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, HEADER_HEIGHT);
    lv_obj_set_style_bg_color(status_bar, theme::COL_HEADER, 0);
    lv_obj_set_style_bg_opa(status_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(status_bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(status_bar, 1, 0);
    lv_obj_set_style_border_color(status_bar, theme::COL_BORDER, 0);

    ble_label = lv_label_create(status_bar);
    lv_obj_set_style_text_color(ble_label, theme::COL_WARNING, 0);
    lv_obj_set_style_text_font(ble_label, &lv_font_montserrat_14, 0);
    lv_label_set_text(ble_label, "BT: Pairing");
    lv_obj_align(ble_label, LV_ALIGN_LEFT_MID, 8, 0);

    action_label = lv_label_create(status_bar);
    lv_obj_set_style_text_color(action_label, theme::COL_MUTED, 0);
    lv_obj_set_style_text_font(action_label, &lv_font_montserrat_14, 0);
    lv_label_set_text(action_label, "");
    lv_obj_align(action_label, LV_ALIGN_RIGHT_MID, -8, 0);

    // ---- Tileview: one swipeable page per config entry ----
    // LV_VER_RES/LV_HOR_RES are runtime calls in LVGL 8 (they read the
    // registered display), so geometry MUST be computed here in build() —
    // a namespace-scope constant would be evaluated before the display is
    // registered and resolve to 0 (this bug once made the content area -64 px
    // tall, hiding every button and panicking the tileview on scroll).
    const lv_coord_t content_height = LV_VER_RES - HEADER_HEIGHT - STATUS_HEIGHT;
    dbg::log("UI geometry: %d x %d, content %d x %d", static_cast<int>(LV_HOR_RES),
             static_cast<int>(LV_VER_RES), static_cast<int>(LV_HOR_RES),
             static_cast<int>(content_height));

    lv_obj_t *content = lv_obj_create(screen);
    lv_obj_remove_style_all(content);
    lv_obj_set_size(content, LV_PCT(100), content_height);
    lv_obj_set_pos(content, 0, HEADER_HEIGHT + STATUS_HEIGHT);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    // Faint static tech grid behind the sliding pages. The pages move, the
    // grid stays — it costs one draw and gives the deck its technical look.
    constexpr lv_coord_t GRID_STEP = 32;
    for (lv_coord_t x = GRID_STEP; x < LV_HOR_RES; x += GRID_STEP) {
        lv_obj_t *line = lv_obj_create(content);
        lv_obj_remove_style_all(line);
        lv_obj_set_size(line, 1, content_height);
        lv_obj_set_pos(line, x, 0);
        lv_obj_set_style_bg_color(line, theme::COL_CARD, 0);
        lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
        lv_obj_clear_flag(line, LV_OBJ_FLAG_CLICKABLE);
    }
    for (lv_coord_t y = GRID_STEP; y < content_height; y += GRID_STEP) {
        lv_obj_t *line = lv_obj_create(content);
        lv_obj_remove_style_all(line);
        lv_obj_set_size(line, LV_HOR_RES, 1);
        lv_obj_set_pos(line, 0, y);
        lv_obj_set_style_bg_color(line, theme::COL_CARD, 0);
        lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
        lv_obj_clear_flag(line, LV_OBJ_FLAG_CLICKABLE);
    }

    tileview = lv_tileview_create(content);
    lv_obj_set_size(tileview, LV_PCT(100), LV_PCT(100));
    // Transparent so the static grid shows through between the cards.
    lv_obj_set_style_bg_opa(tileview, LV_OPA_TRANSP, 0);
    lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(tileview, tileview_changed_event, LV_EVENT_VALUE_CHANGED, nullptr);

    for (size_t page_index = 0; page_index < PAGE_COUNT; ++page_index) {
        lv_obj_t *tile = lv_tileview_add_tile(tileview, static_cast<uint8_t>(page_index), 0, LV_DIR_HOR);
        lv_obj_set_style_pad_all(tile, GRID_GAP, 0);
        tiles[page_index] = tile;

        lv_obj_t *grid = lv_obj_create(tile);
        lv_obj_remove_style_all(grid);
        lv_obj_set_size(grid, LV_PCT(100), LV_PCT(100));
        lv_obj_set_layout(grid, LV_LAYOUT_GRID);
        lv_obj_set_grid_dsc_array(grid, GRID_COLUMNS, GRID_ROWS);
        lv_obj_set_style_pad_column(grid, GRID_GAP, 0);
        lv_obj_set_style_pad_row(grid, GRID_GAP, 0);

        const PageDef &page = PAGES[page_index];
        for (size_t button_index = 0; button_index < page.button_count; ++button_index) {
            const ButtonDef *definition = &page.buttons[button_index];
            lv_obj_t *button = theme::make_button(grid, definition->icon, definition->label, page.accent);
            lv_obj_set_grid_cell(button, LV_GRID_ALIGN_STRETCH,
                                 static_cast<lv_coord_t>(button_index % GRID_COLUMNS_COUNT), 1,
                                 LV_GRID_ALIGN_STRETCH,
                                 static_cast<lv_coord_t>(button_index / GRID_COLUMNS_COUNT), 1);
            lv_obj_add_event_cb(button, button_event, LV_EVENT_CLICKED, const_cast<ButtonDef *>(definition));
        }
    }

    set_active_page(0);
    lv_obj_update_layout(screen);

    // Verify the resolved layout: every geometry value must be positive and
    // inside the panel, or buttons end up invisible (and scrolling broken).
    lv_obj_update_layout(tileview);
    lv_obj_t *first_button = lv_obj_get_child(lv_obj_get_child(tiles[0], 0), 0);
    dbg::log(
        "UI layout: content %dx%d @(%d,%d) | tileview %dx%d | tile0 %dx%d @(%d,%d) | btn %dx%d",
        static_cast<int>(lv_obj_get_width(content)), static_cast<int>(lv_obj_get_height(content)),
        static_cast<int>(lv_obj_get_x(content)), static_cast<int>(lv_obj_get_y(content)),
        static_cast<int>(lv_obj_get_width(tileview)), static_cast<int>(lv_obj_get_height(tileview)),
        first_button != nullptr ? static_cast<int>(lv_obj_get_width(tiles[0])) : -1,
        first_button != nullptr ? static_cast<int>(lv_obj_get_height(tiles[0])) : -1,
        static_cast<int>(lv_obj_get_x(tiles[0])), static_cast<int>(lv_obj_get_y(tiles[0])),
        first_button != nullptr ? static_cast<int>(lv_obj_get_width(first_button)) : -1,
        first_button != nullptr ? static_cast<int>(lv_obj_get_height(first_button)) : -1);

    lv_obj_invalidate(screen);
    lv_timer_create(first_refresh_timer, 200, nullptr);

    last_activity = lv_tick_get();
    // backlight::init() leaves the panel dark; mark the UI dimmed so the first
    // touch (or the end of the splash) explicitly turns it back on.
    dimmed = true;
    lv_timer_create(feedback_timer, 100, nullptr);
    lv_timer_create(ble_poll_timer, BLE_POLL_MS, nullptr);
    lv_timer_create(idle_timer, 1000, nullptr);
}

} // namespace ui
