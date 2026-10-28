#pragma once

#include <lvgl.h>

// =============================================================================
//  Design tokens for the dark "tech" theme + small widget helpers.
// =============================================================================

namespace Theme {
// palette
constexpr uint32_t Bg        = 0x0A0C10;  // near-black blue
constexpr uint32_t Surface   = 0x141922;  // card
constexpr uint32_t SurfaceHi = 0x1C2432;  // card pressed / hover
constexpr uint32_t Stroke    = 0x232B3A;  // hairline borders
constexpr uint32_t Accent    = 0x2F7BFF;  // electric blue
constexpr uint32_t AccentDim = 0x1B3A66;
constexpr uint32_t Good      = 0x00D4AA;  // teal - charging / ok
constexpr uint32_t Warn      = 0xFFB020;  // amber - warnings
constexpr uint32_t Danger    = 0xFF5470;

constexpr uint32_t TextMain  = 0xECF1F8;
constexpr uint32_t TextSub   = 0x7A8699;
constexpr uint32_t TrackBar  = 0x232B3A;

inline lv_color_t C(uint32_t hex) { return lv_color_hex(hex); }

// --- helpers ---------------------------------------------------------------
inline lv_obj_t *label(lv_obj_t *parent, const char *text, const lv_font_t *font,
                       uint32_t color) {
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, C(color), 0);
    lv_obj_set_style_pad_all(l, 0, 0);
    lv_obj_set_style_border_width(l, 0, 0);
    lv_obj_set_style_bg_opa(l, LV_OPA_TRANSP, 0);
    return l;
}

inline lv_obj_t *panel(lv_obj_t *parent, uint32_t bg, uint32_t stroke, uint32_t radius) {
    lv_obj_t *p = lv_obj_create(parent);
    lv_obj_remove_style_all(p);
    lv_obj_set_style_bg_color(p, C(bg), 0);
    lv_obj_set_style_bg_opa(p, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(p, radius, 0);
    if (stroke) {
        lv_obj_set_style_border_color(p, C(stroke), 0);
        lv_obj_set_style_border_width(p, 1, 0);
    } else {
        lv_obj_set_style_border_width(p, 0, 0);
    }
    lv_obj_clear_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    return p;
}

inline lv_obj_t *circleBtn(lv_obj_t *parent, lv_coord_t size, uint32_t bg,
                           const char *symbol, const lv_font_t *font, uint32_t symColor) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_remove_style_all(btn);
    lv_obj_set_size(btn, size, size);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, C(bg), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 24, 0);
    lv_obj_set_style_shadow_color(btn, C(Accent), 0);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_10, 0);
    // pressed feedback
    lv_obj_set_style_bg_color(btn, C(SurfaceHi), LV_STATE_PRESSED);
    lv_obj_t *ic = lv_label_create(btn);
    lv_label_set_text(ic, symbol);
    lv_obj_set_style_text_font(ic, font, 0);
    lv_obj_set_style_text_color(ic, C(symColor), 0);
    lv_obj_center(ic);
    return btn;
}

// 4-bar RSSI signal meter (active bars colored, inactive dim)
inline void rssiBars(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, uint8_t level,
                     uint32_t active, uint32_t idle) {
    static const lv_coord_t heights[4] = {6, 10, 14, 18};
    for (int i = 0; i < 4; i++) {
        lv_obj_t *b = lv_obj_create(parent);
        lv_obj_remove_style_all(b);
        lv_obj_set_size(b, 4, heights[i]);
        lv_obj_set_style_radius(b, 1, 0);
        lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(b, C(i < level ? active : idle), 0);
        lv_obj_set_pos(b, x + i * 7, y - heights[i]);
    }
}
}  // namespace Theme
