#include "image/logo.h"
#include "init_code.h"
#include "pins.h"

#if __has_include("data.h")
#include "data.h"
#define USE_CALIBRATION_DATA 1
#endif

#include "xpt2046.h"

#include "lv_conf.h"
#include "lvgl.h"
#include "neopixel_control.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_vendor.h"

#include <SPI.h>
#include <Arduino.h>

/**
 * Please update the following configuration according to your LCD spec
 */
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (5 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  (1)
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL (!EXAMPLE_LCD_BK_LIGHT_ON_LEVEL)
#define EXAMPLE_PIN_NUM_DATA0          (LCD_DATA0_PIN)
#define EXAMPLE_PIN_NUM_DATA1          (LCD_DATA1_PIN)
#define EXAMPLE_PIN_NUM_DATA2          (LCD_DATA2_PIN)
#define EXAMPLE_PIN_NUM_DATA3          (LCD_DATA3_PIN)
#define EXAMPLE_PIN_NUM_DATA4          (LCD_DATA4_PIN)
#define EXAMPLE_PIN_NUM_DATA5          (LCD_DATA5_PIN)
#define EXAMPLE_PIN_NUM_DATA6          (LCD_DATA6_PIN)
#define EXAMPLE_PIN_NUM_DATA7          (LCD_DATA7_PIN)
#define EXAMPLE_PIN_NUM_PCLK           (PCLK_PIN)
#define EXAMPLE_PIN_NUM_CS             (CS_PIN)
#define EXAMPLE_PIN_NUM_DC             (DC_PIN)
#define EXAMPLE_PIN_NUM_RST            (RST_PIN)
#define EXAMPLE_PIN_NUM_BK_LIGHT       (BK_LIGHT_PIN)

/**
 * The pixel number in horizontal and vertical
 */
#define EXAMPLE_LCD_H_RES              (240)
#define EXAMPLE_LCD_V_RES              (320)

/**
 * Bit number used to represent command and parameter
 */
#define EXAMPLE_LCD_CMD_BITS           (8)
#define EXAMPLE_LCD_PARAM_BITS         (8)

// SPIClass SPI;
XPT2046 touch(SPI, TOUCHSCREEN_CS_PIN, TOUCHSCREEN_IRQ_PIN);
// static bool touch_pin_get_int = false;

#if USE_CALIBRATION_DATA
touch_calibration_t calibration_data[4];
#endif

void print_chip_info(void) {
    Serial.print("Chip: ");
    Serial.println(ESP.getChipModel());
    Serial.print("ChipRevision: ");
    Serial.println(ESP.getChipRevision());
    Serial.print("Psram size: ");
    Serial.print(ESP.getPsramSize() / 1024);
    Serial.println("KB");
    Serial.print("Flash size: ");
    Serial.print(ESP.getFlashChipSize() / 1024);
    Serial.println("KB");
    Serial.print("CPU frequency: ");
    Serial.print(ESP.getCpuFreqMHz());
    Serial.println("MHz");
}

// LED indicator removed; NeoPixel used instead.

// Old LED controller UI removed; NeoPixel controls remain below.

// small struct to hold colorwheel and brightness pointers for the Apply callback
typedef struct { lv_obj_t *colorwheel; lv_obj_t *br; lv_obj_t *preview; lv_obj_t *br_val; lv_obj_t *hex_label; } np_sliders_t;

// Animation state
static bool np_anim_running = false;
static int np_anim_step = 0;
static lv_timer_t *np_anim_timer = NULL;

// color wheel helper (0-255)
static uint32_t Wheel(Adafruit_NeoPixel *s, uint8_t WheelPos) {
    if (!s) return 0;
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85) {
        return s->Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if (WheelPos < 170) {
        WheelPos -= 85;
        return s->Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return s->Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// non-blocking animation timer
static void np_anim_timer_cb(lv_timer_t * timer) {
    if (!np_anim_running) return;
    Adafruit_NeoPixel *s = neopixel_get();
    if (!s) return;
    uint16_t n = s->numPixels();
    for (uint16_t i = 0; i < n; i++) {
        uint8_t pos = (uint8_t)((i * 256 / n) + np_anim_step);
        s->setPixelColor(i, Wheel(s, pos));
    }
    s->show();
    np_anim_step += 4;
}

// Apply callback at file scope
static void apply_cb(lv_event_t * e) {
    np_sliders_t *s = (np_sliders_t*)lv_event_get_user_data(e);
    if (!s) return;
    // read RGB from colorwheel
    lv_color_t c = lv_colorwheel_get_rgb(s->colorwheel);
    uint8_t r = c.ch.red;
    uint8_t g = c.ch.green;
    uint8_t b = c.ch.blue;
    uint32_t br = lv_slider_get_value(s->br);
    neopixel_set_brightness((uint8_t)br);
    neopixel_set_color(r, g, b);
    if (s->preview) lv_obj_set_style_bg_color(s->preview, lv_color_make(r, g, b), LV_PART_MAIN);
    if (s->hex_label) {
        char buf[8];
        sprintf(buf, "#%02X%02X%02X", r, g, b);
        lv_label_set_text(s->hex_label, buf);
    }
    // stop animation when applying a solid color
    np_anim_running = false;
}

// colorwheel change callback to update preview
static void colorwheel_change_cb(lv_event_t * e) {
    np_sliders_t *s = (np_sliders_t*)lv_event_get_user_data(e);
    if (!s) return;
    lv_color_t c = lv_colorwheel_get_rgb(s->colorwheel);
    if (s->preview) lv_obj_set_style_bg_color(s->preview, lv_color_make(c.ch.red, c.ch.green, c.ch.blue), LV_PART_MAIN);
    if (s->hex_label) {
        char buf[8];
        sprintf(buf, "#%02X%02X%02X", c.ch.red, c.ch.green, c.ch.blue);
        lv_label_set_text(s->hex_label, buf);
    }
}

// slider live-preview callback
static void slider_change_cb(lv_event_t * e) {
    // brightness slider changed: update numeric label and preview brightness
    np_sliders_t *s = (np_sliders_t*)lv_event_get_user_data(e);
    if (!s) return;
    uint32_t br = lv_slider_get_value(s->br);
    if (s->br_val) lv_label_set_text_fmt(s->br_val, "%d", br);
    // get current color from colorwheel and scale by brightness
    if (s->colorwheel) {
        lv_color_t c = lv_colorwheel_get_rgb(s->colorwheel);
        uint8_t r = (uint8_t)((uint16_t)c.ch.red * br / 255);
        uint8_t g = (uint8_t)((uint16_t)c.ch.green * br / 255);
        uint8_t b = (uint8_t)((uint16_t)c.ch.blue * br / 255);
        if (s->preview) lv_obj_set_style_bg_color(s->preview, lv_color_make(r, g, b), LV_PART_MAIN);
        if (s->hex_label) {
            char buf[8];
            sprintf(buf, "#%02X%02X%02X", c.ch.red, c.ch.green, c.ch.blue);
            lv_label_set_text(s->hex_label, buf);
        }
    }
}

// preset callbacks
static void preset_off_cb(lv_event_t * e) {
    neopixel_clear();
    np_anim_running = false;
}

static void preset_rainbow_cb(lv_event_t * e) {
    np_anim_running = !np_anim_running;
    if (np_anim_running && !np_anim_timer) {
        np_anim_timer = lv_timer_create(np_anim_timer_cb, 100, NULL);
    }
}

static void preset_solid_cb(lv_event_t * e) {
    // trigger apply_cb to set solid color
    apply_cb(e);
}

// Add pulse preset callback
static void preset_pulse_cb(lv_event_t * e) {
    // Implement pulse animation logic
    np_anim_running = !np_anim_running;
    if (np_anim_running && !np_anim_timer) {
        np_anim_timer = lv_timer_create(np_anim_timer_cb, 100, NULL);
    }
}

// Add chase preset callback
static void preset_chase_cb(lv_event_t * e) {
    // Implement chase animation logic
    np_anim_running = !np_anim_running;
    if (np_anim_running && !np_anim_timer) {
        np_anim_timer = lv_timer_create(np_anim_timer_cb, 100, NULL);
    }
}

void create_ui(void) {
    // ...existing code...
    // Create a main container
    lv_obj_t *main_cont = lv_obj_create(lv_scr_act());
    lv_obj_set_size(main_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_center(main_cont);
    lv_obj_set_style_bg_color(main_cont, lv_color_hex(0x19253A), LV_PART_MAIN);
    lv_obj_set_style_pad_all(main_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(main_cont, 0, LV_PART_MAIN);

    // Title bar
    lv_obj_t *title_bar = lv_obj_create(main_cont);
    lv_obj_set_size(title_bar, LV_PCT(100), 40);
    lv_obj_align(title_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(title_bar, lv_color_hex(0x19253A), LV_PART_MAIN);
    lv_obj_set_style_border_width(title_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(title_bar, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_right(title_bar, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_top(title_bar, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(title_bar, 6, LV_PART_MAIN);

    lv_obj_t *title_label = lv_label_create(title_bar);
    lv_label_set_text(title_label, "Bunty Baba's LED Lab");
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xECEFF4), LV_PART_MAIN);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(title_label, LV_ALIGN_LEFT_MID, 0, 0);

    // Sun icon (use emoji for now)
    lv_obj_t *sun_icon = lv_label_create(title_bar);
    lv_label_set_text(sun_icon, "\xF0\x9F\x8C\x9E"); // Unicode sun emoji
    lv_obj_set_style_text_color(sun_icon, lv_color_hex(0xFFD700), LV_PART_MAIN);
    lv_obj_align(sun_icon, LV_ALIGN_RIGHT_MID, 0, 0);

    // Color wheel centered below title (smaller to fit 240x320)
    lv_obj_t *cw = lv_colorwheel_create(main_cont, true);
    lv_obj_set_size(cw, 140, 140);
    lv_obj_align(cw, LV_ALIGN_TOP_MID, 0, 40);

    // Static Rainbow button below color wheel
    lv_obj_t *btn_rainbow = lv_btn_create(main_cont);
    lv_obj_set_size(btn_rainbow, LV_PCT(80), 44);
    lv_obj_align_to(btn_rainbow, cw, LV_ALIGN_OUT_BOTTOM_MID, 0, 12);
    lv_obj_set_style_bg_color(btn_rainbow, lv_color_hex(0x2E7DD7), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_rainbow, 12, LV_PART_MAIN);
    lv_obj_set_style_text_color(btn_rainbow, lv_color_hex(0xECEFF4), LV_PART_MAIN);
    lv_obj_t *lbl_rainbow = lv_label_create(btn_rainbow);
    lv_label_set_text(lbl_rainbow, "Static Rainbow");
    lv_obj_center(lbl_rainbow);
    lv_obj_add_event_cb(btn_rainbow, preset_rainbow_cb, LV_EVENT_CLICKED, NULL);

    // Horizontal sliders for Brightness and Speed
    lv_obj_t *slider_cont = lv_obj_create(main_cont);
    lv_obj_set_size(slider_cont, LV_PCT(90), 74);
    lv_obj_align_to(slider_cont, btn_rainbow, LV_ALIGN_OUT_BOTTOM_MID, 0, 12);
    lv_obj_set_style_bg_color(slider_cont, lv_color_hex(0x19253A), LV_PART_MAIN);
    lv_obj_set_style_border_width(slider_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(slider_cont, 0, LV_PART_MAIN);

    // Brightness slider
    lv_obj_t *label_br = lv_label_create(slider_cont);
    lv_label_set_text(label_br, "Brightness");
    lv_obj_set_style_text_color(label_br, lv_color_hex(0xECEFF4), LV_PART_MAIN);
    lv_obj_align(label_br, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t *slider_br = lv_slider_create(slider_cont);
    lv_obj_set_size(slider_br, LV_PCT(80), 14);
    lv_obj_align_to(slider_br, label_br, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);
    lv_slider_set_range(slider_br, 0, 255);
    lv_slider_set_value(slider_br, 128, LV_ANIM_OFF);

    // Speed slider
    lv_obj_t *label_speed = lv_label_create(slider_cont);
    lv_label_set_text(label_speed, "Speed");
    lv_obj_set_style_text_color(label_speed, lv_color_hex(0xECEFF4), LV_PART_MAIN);
    lv_obj_align_to(label_speed, slider_br, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 8);
    lv_obj_t *slider_speed = lv_slider_create(slider_cont);
    lv_obj_set_size(slider_speed, LV_PCT(80), 14);
    lv_obj_align_to(slider_speed, label_speed, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);
    lv_slider_set_range(slider_speed, 0, 255);
    lv_slider_set_value(slider_speed, 128, LV_ANIM_OFF);

    // ON button at the bottom
    lv_obj_t *btn_on = lv_btn_create(main_cont);
    lv_obj_set_size(btn_on, LV_PCT(80), 44);
    lv_obj_align(btn_on, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_obj_set_style_bg_color(btn_on, lv_color_hex(0x2E7DD7), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_on, 24, LV_PART_MAIN);
    lv_obj_set_style_text_color(btn_on, lv_color_hex(0xECEFF4), LV_PART_MAIN);
    lv_obj_t *lbl_on = lv_label_create(btn_on);
    lv_label_set_text(lbl_on, "ON");
    lv_obj_center(lbl_on);
    lv_obj_add_event_cb(btn_on, preset_solid_cb, LV_EVENT_CLICKED, NULL);

    // Struct for slider callbacks
    np_sliders_t *sl = (np_sliders_t*)malloc(sizeof(np_sliders_t));
    sl->colorwheel = cw;
    sl->br = slider_br;
    sl->preview = NULL;
    sl->br_val = NULL;
    sl->hex_label = NULL;

    // Auto-apply color changes
    lv_obj_add_event_cb(cw, colorwheel_change_cb, LV_EVENT_VALUE_CHANGED, sl);
    lv_obj_add_event_cb(slider_br, slider_change_cb, LV_EVENT_VALUE_CHANGED, sl);

    // Initialize NeoPixel with default color and brightness
    lv_color_t c_init = lv_colorwheel_get_rgb(cw);
    uint32_t br_init = lv_slider_get_value(slider_br);
    neopixel_set_brightness((uint8_t)br_init);
    neopixel_set_color((uint8_t)c_init.ch.red, (uint8_t)c_init.ch.green, (uint8_t)c_init.ch.blue);
}

void setup() {
    static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
    static lv_disp_drv_t disp_drv;      // contains callback functions
    static lv_indev_drv_t indev_drv;

    Serial.begin(115200);

#if USE_CALIBRATION_DATA
    data_init();
    data_read(calibration_data);
#endif

    SPI.begin(TOUCHSCREEN_SCLK_PIN, TOUCHSCREEN_MISO_PIN, TOUCHSCREEN_MOSI_PIN);
    touch.begin(240, 320);
#if USE_CALIBRATION_DATA
    touch.setCal(calibration_data[0].rawX, calibration_data[2].rawX, calibration_data[0].rawY, calibration_data[2].rawX, 240, 320); // Raw xmin, xmax, ymin, ymax, width, height
#else
    touch.setCal(1788, 285, 1877, 311, 240, 320); // Raw xmin, xmax, ymin, ymax, width, height
    Serial.println("Use default calibration data");
#endif
    touch.setRotation(0);
    print_chip_info();

    // Initialize NeoPixel strip
    neopixel_init();

    pinMode(PWR_EN_PIN, OUTPUT);
    digitalWrite(PWR_EN_PIN, HIGH);
    pinMode(EXAMPLE_PIN_NUM_BK_LIGHT, OUTPUT);
    digitalWrite(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);
    delay(200);

    Serial.println("Initialize Intel 8080 bus");
    esp_lcd_i80_bus_handle_t i80_bus = NULL;
    esp_lcd_i80_bus_config_t bus_config = {
        .dc_gpio_num = EXAMPLE_PIN_NUM_DC,
        .wr_gpio_num = EXAMPLE_PIN_NUM_PCLK,
        .data_gpio_nums = {
            EXAMPLE_PIN_NUM_DATA0,
            EXAMPLE_PIN_NUM_DATA1,
            EXAMPLE_PIN_NUM_DATA2,
            EXAMPLE_PIN_NUM_DATA3,
            EXAMPLE_PIN_NUM_DATA4,
            EXAMPLE_PIN_NUM_DATA5,
            EXAMPLE_PIN_NUM_DATA6,
            EXAMPLE_PIN_NUM_DATA7,
        },
        .bus_width = 8,
        .max_transfer_bytes = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * sizeof(uint16_t)
    };
    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = EXAMPLE_PIN_NUM_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
        .trans_queue_depth = 10,
        .user_ctx = &disp_drv,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1,
        },
        .flags = {
            .swap_color_bytes = 1,
        },
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle));

    Serial.println("Install LCD driver of st7789");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
        .color_space = ESP_LCD_COLOR_SPACE_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    // Send all the commands
    for (int i = 0; i < sizeof(st_init_cmds) / sizeof(lcd_init_cmd_t); i ++)
    {
        esp_lcd_panel_io_tx_param(io_handle, st_init_cmds[i].cmd, st_init_cmds[i].data, st_init_cmds[i].len);
        if (st_init_cmds[i].cmd & 0x11) {
            delay(st_init_cmds[i].delay);
        }
    }

    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES, gImage_logo);
    delay(3000);

    lv_init();
    // alloc draw buffers used by LVGL from PSRAM
    lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    assert(buf1);
    lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    assert(buf2);
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES);

    Serial.println("Register display driver to LVGL");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    disp_drv.flush_cb = example_lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    lv_disp_drv_register(&disp_drv);

    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lv_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    create_ui();
}


// LVGL display flush callback: forward buffered area to the ESP LCD panel
static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
    lv_disp_flush_ready(drv);
}


// Touch read callback for LVGL input driver
static void lv_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    uint16_t x, y;
    if (touch.pressed()) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touch.X();
        data->point.y = touch.Y();
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}


// duplicate setup stub removed

void loop() {
    delay(2);
    lv_timer_handler();
}
