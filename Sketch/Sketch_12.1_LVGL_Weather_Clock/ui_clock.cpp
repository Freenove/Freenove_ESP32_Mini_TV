#include "ui_clock.h"
#include "config_store.h"
#include "ntp_time.h"
#include "ui_analog_dial.h"
#include "ui_analog_hands.h"
#include "ui_font_clock.h"
#include "ui_freenove.h"
#include "ui_icons.h"
#include "ui_logo.h"
#include "ui_xl_digits.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* Debug **/
#ifndef UI_CLOCK_FORCE_0000
#define UI_CLOCK_FORCE_0000 0
#endif

/* Theme palettes */
struct ThemePalette {
    uint32_t bg;
    uint32_t text;
    uint32_t hour;
    uint32_t min;
    uint32_t temp_bar;
    uint32_t hum_bar;
    uint32_t bar_track;
    uint32_t hint;
    uint32_t ampm_fg;
    uint32_t step_idle;
    uint32_t step_active;
    /* Analog dial */
    uint32_t dial_ring;
    uint32_t dial_tick;
    uint32_t dial_hand;
    uint32_t dial_sec;
    uint32_t dial_cap;
    /* XL stacked digital face */
    uint32_t xl_hour;
    uint32_t xl_min;
};

static const ThemePalette PALETTE_DARK = {
    0x000000, /* bg */
    0xFFFFFF, /* text */
    0x5EC8F0, /* hour */
    0xF5D76E, /* min */
    0xFF6B9D, /* temp_bar */
    0x4A9EFF, /* hum_bar */
    0x2A2A2A, /* bar_track */
    0x888888, /* hint */
    0x000000, /* ampm_fg */
    0x444444, /* step_idle */
    0x5EC8F0, /* step_active */
    0x3A3A3C, /* dial_ring */
    0xF2F2F7, /* dial_tick */
    0xF2F2F7, /* dial_hand */
    0xC7C7CC, /* dial_sec */
    0x8E8E93, /* dial_cap */
    0xF2D0C0, /* xl_hour */
    0xE85A6A, /* xl_min */
};

static const ThemePalette PALETTE_LIGHT = {
    0xF2F4F7, /* bg */
    0x1A1A1A, /* text */
    0x1A8BB8, /* hour */
    0xC9A227, /* min */
    0xE85A8A, /* temp_bar */
    0x3B82F6, /* hum_bar */
    0xD0D5DC, /* bar_track */
    0x6A6A6A, /* hint */
    0x1A1A1A, /* ampm_fg */
    0xC5CAD3, /* step_idle */
    0x1A8BB8, /* step_active */
    0xC7C7CC, /* dial_ring */
    0x1C1C1E, /* dial_tick */
    0x1C1C1E, /* dial_hand */
    0x636366, /* dial_sec */
    0x636366, /* dial_cap */
    0xC08070, /* xl_hour */
    0xC44050, /* xl_min */
};

static uint8_t s_theme = THEME_DARK;
static uint8_t s_ui_face = UI_FACE_WEATHER;
static const ThemePalette *pal(void) {
    return (s_theme == THEME_LIGHT) ? &PALETTE_LIGHT : &PALETTE_DARK;
}

/* Boot-page widgets */
static lv_obj_t *scr = nullptr;
static lv_obj_t *boot_bg = nullptr;
static lv_obj_t *boot_title = nullptr;
static lv_obj_t *boot_subtitle = nullptr;
static lv_obj_t *boot_label = nullptr;
static lv_obj_t *boot_hint = nullptr;
static lv_obj_t *boot_bar = nullptr;
static lv_obj_t *boot_step_dots[BOOT_PAGE_COUNT] = {nullptr};
static BootPage s_boot_page = BOOT_PAGE_START;
static bool s_boot_visible = false;

/* Long-press WiFi reconfig warning overlay */
static lv_obj_t *reconfig_panel = nullptr;
static lv_obj_t *reconfig_countdown = nullptr;
static lv_obj_t *reconfig_label = nullptr;

static void set_weather_face_visible(bool visible);
static void apply_face_visibility(void);
static void hide_or_show(lv_obj_t *o, bool visible);

/* Main UI widgets */
static lv_obj_t *city_label = nullptr;
static lv_obj_t *cond_label = nullptr;
static lv_obj_t *weather_img = nullptr;

static lv_obj_t *hour_label = nullptr;
static lv_obj_t *colon_label = nullptr;
static lv_obj_t *min_label = nullptr;
static lv_obj_t *ampm_box = nullptr;
static lv_obj_t *ampm_label = nullptr;
static lv_obj_t *sec_label = nullptr;
static lv_obj_t *date_label = nullptr;

static lv_obj_t *temp_bar = nullptr;
static lv_obj_t *hum_bar = nullptr;
static lv_obj_t *temp_value = nullptr;
static lv_obj_t *hum_value = nullptr;
static lv_obj_t *temp_img = nullptr;
static lv_obj_t *hum_img = nullptr;
static lv_obj_t *logo_img = nullptr;
static lv_obj_t *freenove_img = nullptr;

/* Analog face widgets */
static lv_obj_t *analog_root = nullptr;
static lv_obj_t *analog_dial_img = nullptr;
static lv_obj_t *analog_hour_hand = nullptr;
static lv_obj_t *analog_min_hand = nullptr;
static lv_obj_t *analog_sec_line = nullptr;
static lv_obj_t *analog_cap = nullptr;
static lv_obj_t *analog_wx_label = nullptr;
static lv_obj_t *analog_date_label = nullptr;
static lv_obj_t *analog_logo_img = nullptr;
static lv_obj_t *analog_freenove_img = nullptr;

/* XL stacked digital face */
static lv_obj_t *xl_root = nullptr;
static lv_obj_t *xl_h0 = nullptr;
static lv_obj_t *xl_h1 = nullptr;
static lv_obj_t *xl_m0 = nullptr;
static lv_obj_t *xl_m1 = nullptr;

static lv_point_precise_t s_sec_pts[2];

static const int ANALOG_CX = 120;
static const int ANALOG_CY = 114;
static const int SEC_LEN = 100;
static const int SEC_BACK = 18;

/** Logo rotation callbac **/
static void logo_rotate_exec(void *obj, int32_t angle) {
    lv_image_set_rotation((lv_obj_t *)obj, (int32_t)angle);
}

/* Fonts */
static const lv_font_t *font_xs = nullptr;
static const lv_font_t *font_sm = nullptr;
static const lv_font_t *font_md = nullptr;
static const lv_font_t *font_lg = nullptr;
static const lv_font_t *font_xl = nullptr;
static const lv_font_t *font_clock_lg = nullptr;
static const lv_font_t *font_clock_sm = nullptr;

/** Select built-in fonts and optional custom clock fonts. */
static void pick_fonts(void) {
#if LV_FONT_MONTSERRAT_12
    font_xs = &lv_font_montserrat_12;
#else
    font_xs = LV_FONT_DEFAULT;
#endif
#if LV_FONT_MONTSERRAT_14
    font_sm = &lv_font_montserrat_14;
#else
    font_sm = LV_FONT_DEFAULT;
#endif
#if LV_FONT_MONTSERRAT_16
    font_md = &lv_font_montserrat_16;
#else
    font_md = font_sm;
#endif
#if LV_FONT_MONTSERRAT_28
    font_lg = &lv_font_montserrat_28;
#elif LV_FONT_MONTSERRAT_24
    font_lg = &lv_font_montserrat_24;
#else
    font_lg = font_md;
#endif
#if LV_FONT_MONTSERRAT_48
    font_xl = &lv_font_montserrat_48;
#elif LV_FONT_MONTSERRAT_42
    font_xl = &lv_font_montserrat_42;
#elif LV_FONT_MONTSERRAT_40
    font_xl = &lv_font_montserrat_40;
#elif LV_FONT_MONTSERRAT_36
    font_xl = &lv_font_montserrat_36;
#elif LV_FONT_MONTSERRAT_32
    font_xl = &lv_font_montserrat_32;
#else
    font_xl = font_lg;
#endif

#if UI_CLOCK_USE_CUSTOM_FONT
    font_clock_lg = UI_FONT_CLOCK_LG;
    font_clock_sm = UI_FONT_CLOCK_SM;
#else
    font_clock_lg = font_xl;
    font_clock_sm = font_md;
#endif
}

/** Transparent background label. */
static lv_obj_t *make_label(lv_obj_t *parent, const lv_font_t *font, uint32_t color) {
    lv_obj_t *lb = lv_label_create(parent);
    lv_obj_set_style_text_font(lb, font, 0);
    lv_obj_set_style_text_color(lb, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(lb, LV_OPA_TRANSP, 0);
    return lb;
}

/** Seconds stay unscaled. */
static void scale_hm_label(lv_obj_t *lb, int32_t scale_256) {
    if (lb == nullptr) return;
    lv_obj_set_style_transform_pivot_x(lb, 0, 0);
    lv_obj_set_style_transform_pivot_y(lb, 0, 0);
    lv_obj_set_style_transform_scale(lb, scale_256, 0);
}

/** Solid screen background; disable scroll. */
static void style_screen(lv_obj_t *obj) {
    lv_obj_set_style_bg_color(obj, lv_color_hex(pal()->bg), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

/** tm_mon (0-11) to English month abbreviation. */
static const char *month_name(int mon) {
    static const char *names[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    if (mon < 0 || mon > 11) return "???";
    return names[mon];
}

/** tm_wday (0=Sun) to English weekday abbreviation. */
static const char *wday_name(int wday) {
    static const char *names[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    if (wday < 0 || wday > 6) return "???";
    return names[wday];
}

static void set_weather_icon(int code) {
    if (weather_img == nullptr) return;
    lv_image_set_src(weather_img, ui_icon_weather_by_code(code));
}

static const char *boot_page_title(BootPage page) {
    switch (page) {
        case BOOT_PAGE_START:   return "System Boot";
        case BOOT_PAGE_WIFI:    return "Network";
        case BOOT_PAGE_NTP:     return "Time Sync";
        case BOOT_PAGE_WEATHER: return "Weather";
        case BOOT_PAGE_READY:   return "Almost Ready";
        default:                return "Loading";
    }
}

static const char *boot_page_subtitle(BootPage page) {
    switch (page) {
        case BOOT_PAGE_START:   return "Step 1 / 5";
        case BOOT_PAGE_WIFI:    return "Step 2 / 5";
        case BOOT_PAGE_NTP:     return "Step 3 / 5";
        case BOOT_PAGE_WEATHER: return "Step 4 / 5";
        case BOOT_PAGE_READY:   return "Step 5 / 5";
        default:                return "";
    }
}

static int boot_page_progress(BootPage page) {
    return ((int)page + 1) * 100 / (int)BOOT_PAGE_COUNT;
}

/** Icon + bar + value row. */
static void make_bar_row(int y, const lv_image_dsc_t *icon_dsc,
                         uint32_t bar_color, lv_obj_t **img_out,
                         lv_obj_t **bar_out, lv_obj_t **val_out) {
    const int bar_w = 70;

    lv_obj_t *img = lv_image_create(scr);
    lv_image_set_src(img, icon_dsc);

    int iw = (int)icon_dsc->header.w;
    int ih = (int)icon_dsc->header.h;
    if (iw > UI_ICON_METRIC_W && iw > 0) {
        lv_image_set_scale(img, (256 * UI_ICON_METRIC_W) / iw);
        iw = UI_ICON_METRIC_W;
        ih = UI_ICON_METRIC_H;
    }
    lv_obj_set_pos(img, 8, y);
    *img_out = img;

    lv_obj_t *bar = lv_bar_create(scr);
    lv_obj_set_size(bar, bar_w, 10);
    lv_obj_set_pos(bar, 8 + iw + 6, y + (ih > 10 ? (ih - 10) / 2 : 0));
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(pal()->bar_track), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 4, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_color_hex(bar_color), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 4, LV_PART_INDICATOR);
    *bar_out = bar;

    lv_obj_t *val = make_label(scr, font_sm, pal()->text);
    lv_label_set_text(val, "--");
    lv_obj_set_pos(val, 8 + iw + 6 + bar_w + 8, y + 2);
    *val_out = val;
}

static void ensure_boot_widgets(void) {
    if (boot_title != nullptr) {
        return;
    }

    pick_fonts();
    scr = lv_screen_active();
    style_screen(scr);

    /* Full-screen backdrop so boot covers the main clock UI. */
    boot_bg = lv_obj_create(scr);
    lv_obj_set_size(boot_bg, 240, 240);
    lv_obj_set_pos(boot_bg, 0, 0);
    lv_obj_set_style_bg_color(boot_bg, lv_color_hex(pal()->bg), 0);
    lv_obj_set_style_bg_opa(boot_bg, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(boot_bg, 0, 0);
    lv_obj_set_style_radius(boot_bg, 0, 0);
    lv_obj_set_style_pad_all(boot_bg, 0, 0);
    lv_obj_clear_flag(boot_bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(boot_bg, LV_OBJ_FLAG_HIDDEN);

    const int dot_n = (int)BOOT_PAGE_COUNT;
    const int dot_sz = 7;
    const int gap = 8;
    const int total_w = dot_n * dot_sz + (dot_n - 1) * gap;
    const int start_x = (240 - total_w) / 2;
    for (int i = 0; i < dot_n; ++i) {
        lv_obj_t *dot = lv_obj_create(scr);
        lv_obj_set_size(dot, dot_sz, dot_sz);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_set_style_pad_all(dot, 0, 0);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_pos(dot, start_x + i * (dot_sz + gap), 20);
        boot_step_dots[i] = dot;
    }

    boot_title = make_label(scr, font_md, pal()->hour);
    lv_obj_set_style_text_align(boot_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(boot_title, 220);
    lv_label_set_text(boot_title, "Weather Clock");
    lv_obj_align(boot_title, LV_ALIGN_TOP_MID, 0, 40);

    boot_subtitle = make_label(scr, font_sm, pal()->min);
    lv_obj_set_style_text_align(boot_subtitle, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(boot_subtitle, 220);
    lv_label_set_text(boot_subtitle, "Initializing");
    lv_obj_align(boot_subtitle, LV_ALIGN_TOP_MID, 0, 64);

    boot_bar = lv_bar_create(scr);
    lv_obj_set_size(boot_bar, 160, 6);
    lv_obj_align(boot_bar, LV_ALIGN_TOP_MID, 0, 90);
    lv_bar_set_range(boot_bar, 0, 100);
    lv_bar_set_value(boot_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(boot_bar, lv_color_hex(pal()->bar_track), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(boot_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(boot_bar, 3, LV_PART_MAIN);
    lv_obj_set_style_bg_color(boot_bar, lv_color_hex(pal()->hour), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(boot_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(boot_bar, 3, LV_PART_INDICATOR);

    boot_label = make_label(scr, font_sm, pal()->text);
    lv_obj_set_style_text_align(boot_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(boot_label, 220);
    lv_obj_set_height(boot_label, 92);
    lv_label_set_long_mode(boot_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_line_space(boot_label, 6, 0);
    lv_obj_align(boot_label, LV_ALIGN_TOP_MID, 0, 110);

    boot_hint = make_label(scr, font_sm, pal()->hint);
    lv_obj_set_style_text_align(boot_hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(boot_hint, 220);
    lv_label_set_text(boot_hint, "Please wait...");
    lv_obj_align(boot_hint, LV_ALIGN_BOTTOM_MID, 0, -18);
}

static void boot_raise_chrome(void) {
    if (boot_bg) {
        lv_obj_move_foreground(boot_bg);
    }
    for (int i = 0; i < (int)BOOT_PAGE_COUNT; ++i) {
        if (boot_step_dots[i]) {
            lv_obj_move_foreground(boot_step_dots[i]);
        }
    }
    if (boot_title) lv_obj_move_foreground(boot_title);
    if (boot_subtitle) lv_obj_move_foreground(boot_subtitle);
    if (boot_bar) lv_obj_move_foreground(boot_bar);
    if (boot_label) lv_obj_move_foreground(boot_label);
    if (boot_hint) lv_obj_move_foreground(boot_hint);
}

static void hide_or_show(lv_obj_t *o, bool visible) {
    if (!o) return;
    if (visible) {
        lv_obj_clear_flag(o, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
    }
}

static void set_boot_visible(bool visible) {
    s_boot_visible = visible;
    hide_or_show(boot_bg, visible);
    hide_or_show(boot_title, visible);
    hide_or_show(boot_subtitle, visible);
    hide_or_show(boot_label, visible);
    hide_or_show(boot_hint, visible);
    hide_or_show(boot_bar, visible);
    for (int i = 0; i < (int)BOOT_PAGE_COUNT; ++i) {
        hide_or_show(boot_step_dots[i], visible);
    }
    if (visible) {
        /* Cover any main-clock widgets while portal / boot is up. */
        set_weather_face_visible(false);
        hide_or_show(analog_root, false);
        hide_or_show(xl_root, false);
        boot_raise_chrome();
    } else if (city_label != nullptr || analog_root != nullptr || xl_root != nullptr) {
        apply_face_visibility();
    }
}

static void format_date_line(char *out, size_t out_len, const struct tm &t,
                             uint8_t date_order, uint8_t month_en);

/** Second-hand needle: deg=0 at 12 o'clock, clockwise. */
static void set_sec_points(float deg) {
    const float rad = (deg - 90.0f) * (float)M_PI / 180.0f;
    const float c = cosf(rad);
    const float s = sinf(rad);
    s_sec_pts[0].x = (lv_value_precise_t)(ANALOG_CX - c * (float)SEC_BACK);
    s_sec_pts[0].y = (lv_value_precise_t)(ANALOG_CY - s * (float)SEC_BACK);
    s_sec_pts[1].x = (lv_value_precise_t)(ANALOG_CX + c * (float)SEC_LEN);
    s_sec_pts[1].y = (lv_value_precise_t)(ANALOG_CY + s * (float)SEC_LEN);
}

static void apply_analog_image_recolor(lv_obj_t *img) {
    if (img == nullptr) return;
    if (s_theme == THEME_LIGHT) {
        lv_obj_set_style_image_recolor(img, lv_color_hex(0x1C1C1E), 0);
        lv_obj_set_style_image_recolor_opa(img, LV_OPA_COVER, 0);
    } else {
        lv_obj_set_style_image_recolor_opa(img, LV_OPA_TRANSP, 0);
    }
}

static void apply_logo_recolor(lv_obj_t *img);

static void set_hand_image_angle(lv_obj_t *img, float deg) {
    if (img == nullptr) return;
    lv_image_set_rotation(img, (int32_t)(deg * 10.0f + 0.5f));
}

static void refresh_analog_theme_colors(void) {
    if (analog_root == nullptr) return;
    const ThemePalette *p = pal();

    apply_analog_image_recolor(analog_dial_img);
    apply_analog_image_recolor(analog_hour_hand);
    apply_analog_image_recolor(analog_min_hand);
    apply_logo_recolor(analog_logo_img);
    apply_logo_recolor(analog_freenove_img);

    if (analog_sec_line) {
        lv_obj_set_style_line_color(analog_sec_line, lv_color_hex(p->dial_sec), 0);
    }
    if (analog_cap) {
        lv_obj_set_style_bg_color(analog_cap, lv_color_hex(p->dial_cap), 0);
    }
    if (analog_wx_label) {
        lv_obj_set_style_text_color(analog_wx_label, lv_color_hex(p->hint), 0);
    }
    if (analog_date_label) {
        lv_obj_set_style_text_color(analog_date_label, lv_color_hex(p->hint), 0);
    }
}

static void update_analog_weather(const WeatherInfo &w) {
    if (analog_wx_label == nullptr) return;

    const char *cond = "--";
    if (w.valid) {
        cond = w.condition[0] ? w.condition : weather_condition_text(w.weather_code);
        if (!cond || !cond[0]) {
            cond = "--";
        }
    }
    lv_label_set_text(analog_wx_label, cond);
    lv_obj_align(analog_wx_label, LV_ALIGN_BOTTOM_LEFT, 6, -4);
}

static void update_analog_hands(const struct tm &t) {
    if (analog_hour_hand == nullptr) return;

    const float sec_deg = (float)t.tm_sec * 6.0f;
    const float min_deg = (float)t.tm_min * 6.0f + (float)t.tm_sec * 0.1f;
    const float hour_deg = (float)(t.tm_hour % 12) * 30.0f + (float)t.tm_min * 0.5f;

    set_hand_image_angle(analog_hour_hand, hour_deg);
    set_hand_image_angle(analog_min_hand, min_deg);

    set_sec_points(sec_deg);
    lv_line_set_points(analog_sec_line, s_sec_pts, 2);

    if (analog_date_label) {
        const AppConfig &cfg = config_store_get();
        char date_buf[40];
        format_date_line(date_buf, sizeof(date_buf), t, cfg.date_order, cfg.month_en);
        lv_label_set_text(analog_date_label, date_buf);
        lv_obj_align(analog_date_label, LV_ALIGN_BOTTOM_RIGHT, -6, -4);
    }
}

/** center logo/Freenove, corners weather/date. **/
static void create_analog_face(void) {
    if (analog_root != nullptr) return;

    const ThemePalette *p = pal();

    analog_root = lv_obj_create(scr);
    lv_obj_set_size(analog_root, 240, 240);
    lv_obj_set_pos(analog_root, 0, 0);
    lv_obj_set_style_bg_opa(analog_root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(analog_root, 0, 0);
    lv_obj_set_style_pad_all(analog_root, 0, 0);
    lv_obj_clear_flag(analog_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(analog_root, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    analog_dial_img = lv_image_create(analog_root);
    lv_image_set_src(analog_dial_img, &icon_analog_dial);
    lv_obj_set_pos(analog_dial_img, 0, 0);

    analog_logo_img = lv_image_create(analog_root);
    lv_image_set_src(analog_logo_img, &icon_logo);
    lv_image_set_pivot(analog_logo_img, UI_LOGO_W / 2, UI_LOGO_H / 2);
    lv_image_set_scale(analog_logo_img, 120);
    lv_obj_set_pos(analog_logo_img,
                   ANALOG_CX - UI_LOGO_W / 2,
                   ANALOG_CY - 45 - UI_LOGO_H / 2);

    analog_freenove_img = lv_image_create(analog_root);
    lv_image_set_src(analog_freenove_img, &icon_freenove);
    lv_image_set_scale(analog_freenove_img, 256);
    lv_obj_set_pos(analog_freenove_img,
                   ANALOG_CX - UI_FREENOVE_W / 2,
                   ANALOG_CY - 26);

    analog_hour_hand = lv_image_create(analog_root);
    lv_image_set_src(analog_hour_hand, &icon_analog_hour);
    lv_image_set_pivot(analog_hour_hand, UI_ANALOG_HOUR_W / 2, UI_ANALOG_HOUR_PIVOT_Y);
    lv_obj_set_pos(analog_hour_hand,
                   ANALOG_CX - UI_ANALOG_HOUR_W / 2,
                   ANALOG_CY - UI_ANALOG_HOUR_PIVOT_Y);

    analog_min_hand = lv_image_create(analog_root);
    lv_image_set_src(analog_min_hand, &icon_analog_min);
    lv_image_set_pivot(analog_min_hand, UI_ANALOG_MIN_W / 2, UI_ANALOG_MIN_PIVOT_Y);

    lv_obj_set_pos(analog_min_hand,
                   ANALOG_CX - UI_ANALOG_MIN_W / 2 - 1,
                   ANALOG_CY - UI_ANALOG_MIN_PIVOT_Y - 1);

    set_sec_points(0);
    analog_sec_line = lv_line_create(analog_root);
    lv_line_set_points(analog_sec_line, s_sec_pts, 2);
    lv_obj_set_style_line_width(analog_sec_line, 1, 0);
    lv_obj_set_style_line_color(analog_sec_line, lv_color_hex(p->dial_sec), 0);
    lv_obj_set_style_line_rounded(analog_sec_line, true, 0);
    lv_obj_clear_flag(analog_sec_line, LV_OBJ_FLAG_CLICKABLE);

    analog_cap = lv_obj_create(analog_root);
    lv_obj_set_size(analog_cap, 14, 14);
    lv_obj_set_style_radius(analog_cap, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(analog_cap, lv_color_hex(p->dial_cap), 0);
    lv_obj_set_style_bg_opa(analog_cap, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(analog_cap, 0, 0);
    lv_obj_set_style_pad_all(analog_cap, 0, 0);
    lv_obj_clear_flag(analog_cap, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(analog_cap, ANALOG_CX - 7, ANALOG_CY - 7);

    analog_wx_label = make_label(analog_root, font_xs, p->hint);
    lv_obj_set_style_text_align(analog_wx_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_width(analog_wx_label, 118);
    lv_label_set_long_mode(analog_wx_label, LV_LABEL_LONG_CLIP);
    lv_label_set_text(analog_wx_label, "--");
    lv_obj_align(analog_wx_label, LV_ALIGN_BOTTOM_LEFT, 6, -4);

    analog_date_label = make_label(analog_root, font_xs, p->hint);
    lv_obj_set_style_text_align(analog_date_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_width(analog_date_label, 118);
    lv_label_set_long_mode(analog_date_label, LV_LABEL_LONG_CLIP);
    lv_label_set_text(analog_date_label, "--");
    lv_obj_align(analog_date_label, LV_ALIGN_BOTTOM_RIGHT, -6, -4);

    apply_analog_image_recolor(analog_dial_img);
    apply_analog_image_recolor(analog_hour_hand);
    apply_analog_image_recolor(analog_min_hand);
    apply_logo_recolor(analog_logo_img);
    apply_logo_recolor(analog_freenove_img);
    set_hand_image_angle(analog_hour_hand, 0);
    set_hand_image_angle(analog_min_hand, 0);
}

static void set_weather_face_visible(bool visible) {
    hide_or_show(city_label, visible);
    hide_or_show(cond_label, visible);
    hide_or_show(weather_img, visible);
    hide_or_show(hour_label, visible);
    hide_or_show(colon_label, visible);
    hide_or_show(min_label, visible);
    hide_or_show(ampm_box, visible);
    hide_or_show(sec_label, visible);
    hide_or_show(date_label, visible);
    hide_or_show(temp_bar, visible);
    hide_or_show(hum_bar, visible);
    hide_or_show(temp_value, visible);
    hide_or_show(hum_value, visible);
    hide_or_show(temp_img, visible);
    hide_or_show(hum_img, visible);
    hide_or_show(logo_img, visible);
    hide_or_show(freenove_img, visible);
}

static void apply_face_visibility(void) {
    const bool weather = (s_ui_face == UI_FACE_WEATHER);
    const bool analog = (s_ui_face == UI_FACE_ANALOG);
    const bool xl = (s_ui_face == UI_FACE_XL);
    set_weather_face_visible(weather);
    hide_or_show(analog_root, analog);
    hide_or_show(xl_root, xl);
}

static void apply_xl_digit_recolor(lv_obj_t *img, uint32_t color) {
    if (img == nullptr) return;
    lv_obj_set_style_image_recolor(img, lv_color_hex(color), 0);
    lv_obj_set_style_image_recolor_opa(img, LV_OPA_COVER, 0);
}

static void refresh_xl_theme_colors(void) {
    if (xl_h0 == nullptr) return;
    const ThemePalette *p = pal();
    apply_xl_digit_recolor(xl_h0, p->xl_hour);
    apply_xl_digit_recolor(xl_h1, p->xl_hour);
    apply_xl_digit_recolor(xl_m0, p->xl_min);
    apply_xl_digit_recolor(xl_m1, p->xl_min);
}

static void set_xl_digit(lv_obj_t *img, int digit) {
    if (img == nullptr) return;
    lv_image_set_src(img, ui_xl_digit(digit));
}

static void update_xl_time(const struct tm &t) {
    if (xl_h0 == nullptr) return;

    const AppConfig &cfg = config_store_get();
    int display_hour = t.tm_hour;
    if (cfg.hour12 == HOUR_FMT_12) {
        display_hour %= 12;
        if (display_hour == 0) {
            display_hour = 12;
        }
    }

    set_xl_digit(xl_h0, display_hour / 10);
    set_xl_digit(xl_h1, display_hour % 10);
    set_xl_digit(xl_m0, t.tm_min / 10);
    set_xl_digit(xl_m1, t.tm_min % 10);
}

static void create_xl_face(void) {
    if (xl_root != nullptr) return;

    const ThemePalette *p = pal();
    const int x0 = 0;
    const int y_hour = UI_XL_DIGIT_TOP;
    const int y_min = UI_XL_DIGIT_TOP + UI_XL_DIGIT_H + UI_XL_DIGIT_GAP_Y;

    xl_root = lv_obj_create(scr);
    lv_obj_set_size(xl_root, 240, 240);
    lv_obj_set_pos(xl_root, 0, 0);
    lv_obj_set_style_bg_opa(xl_root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(xl_root, 0, 0);
    lv_obj_set_style_pad_all(xl_root, 0, 0);
    lv_obj_clear_flag(xl_root, LV_OBJ_FLAG_SCROLLABLE);

    xl_h0 = lv_image_create(xl_root);
    xl_h1 = lv_image_create(xl_root);
    xl_m0 = lv_image_create(xl_root);
    xl_m1 = lv_image_create(xl_root);

    set_xl_digit(xl_h0, 0);
    set_xl_digit(xl_h1, 0);
    set_xl_digit(xl_m0, 0);
    set_xl_digit(xl_m1, 0);

    lv_obj_set_pos(xl_h0, x0, y_hour);
    lv_obj_set_pos(xl_h1, x0 + UI_XL_DIGIT_W + UI_XL_DIGIT_GAP_X, y_hour);
    lv_obj_set_pos(xl_m0, x0, y_min);
    lv_obj_set_pos(xl_m1, x0 + UI_XL_DIGIT_W + UI_XL_DIGIT_GAP_X, y_min);

    apply_xl_digit_recolor(xl_h0, p->xl_hour);
    apply_xl_digit_recolor(xl_h1, p->xl_hour);
    apply_xl_digit_recolor(xl_m0, p->xl_min);
    apply_xl_digit_recolor(xl_m1, p->xl_min);
}

static void refresh_boot_theme_colors(void) {
    if (boot_title == nullptr) return;
    const ThemePalette *p = pal();

    style_screen(scr);
    if (boot_bg) {
        lv_obj_set_style_bg_color(boot_bg, lv_color_hex(p->bg), 0);
        lv_obj_set_style_bg_opa(boot_bg, LV_OPA_COVER, 0);
    }
    lv_obj_set_style_text_color(boot_title, lv_color_hex(p->hour), 0);
    lv_obj_set_style_text_color(boot_subtitle, lv_color_hex(p->min), 0);
    lv_obj_set_style_text_color(boot_label, lv_color_hex(p->text), 0);
    if (boot_hint) {
        lv_obj_set_style_text_color(boot_hint, lv_color_hex(p->hint), 0);
    }
    if (boot_bar) {
        lv_obj_set_style_bg_color(boot_bar, lv_color_hex(p->bar_track), LV_PART_MAIN);
        lv_obj_set_style_bg_color(boot_bar, lv_color_hex(p->hour), LV_PART_INDICATOR);
    }
    for (int i = 0; i < (int)BOOT_PAGE_COUNT; ++i) {
        if (!boot_step_dots[i]) continue;
        uint32_t c = (i == (int)s_boot_page) ? p->step_active : p->step_idle;
        lv_obj_set_style_bg_color(boot_step_dots[i], lv_color_hex(c), 0);
    }
}

/** Recolor white ARGB icons for light theme. **/
static void apply_icon_recolor(lv_obj_t *img) {
    if (img == nullptr) return;
    if (s_theme == THEME_LIGHT) {
        lv_obj_set_style_image_recolor(img, lv_color_hex(0x1A1A1A), 0);
        lv_obj_set_style_image_recolor_opa(img, LV_OPA_COVER, 0);
    } else {
        lv_obj_set_style_image_recolor_opa(img, LV_OPA_TRANSP, 0);
    }
}

static void apply_logo_recolor(lv_obj_t *img) {
    if (img == nullptr) return;
    if (s_theme == THEME_LIGHT) {
        lv_obj_set_style_image_recolor(img, lv_color_hex(0x1A1A1A), 0);
    } else {
        lv_obj_set_style_image_recolor(img, lv_color_hex(0xFFFFFF), 0);
    }
    lv_obj_set_style_image_recolor_opa(img, LV_OPA_COVER, 0);
}

static void refresh_main_theme_colors(void) {
    if (city_label == nullptr) return;
    const ThemePalette *p = pal();

    style_screen(scr);
    lv_obj_set_style_text_color(city_label, lv_color_hex(p->text), 0);
    lv_obj_set_style_text_color(cond_label, lv_color_hex(p->text), 0);
    lv_obj_set_style_text_color(hour_label, lv_color_hex(p->hour), 0);
    if (colon_label) {
        lv_obj_set_style_text_color(colon_label, lv_color_hex(p->text), 0);
    }
    lv_obj_set_style_text_color(min_label, lv_color_hex(p->min), 0);
    lv_obj_set_style_text_color(sec_label, lv_color_hex(p->hour), 0);
    lv_obj_set_style_text_color(date_label, lv_color_hex(p->text), 0);
    lv_obj_set_style_text_color(temp_value, lv_color_hex(p->text), 0);
    lv_obj_set_style_text_color(hum_value, lv_color_hex(p->text), 0);

    if (ampm_box) {
        lv_obj_set_style_bg_color(ampm_box, lv_color_hex(p->min), 0);
    }
    if (ampm_label) {
        lv_obj_set_style_text_color(ampm_label, lv_color_hex(p->ampm_fg), 0);
    }
    if (temp_bar) {
        lv_obj_set_style_bg_color(temp_bar, lv_color_hex(p->bar_track), LV_PART_MAIN);
        lv_obj_set_style_bg_color(temp_bar, lv_color_hex(p->temp_bar), LV_PART_INDICATOR);
    }
    if (hum_bar) {
        lv_obj_set_style_bg_color(hum_bar, lv_color_hex(p->bar_track), LV_PART_MAIN);
        lv_obj_set_style_bg_color(hum_bar, lv_color_hex(p->hum_bar), LV_PART_INDICATOR);
    }

    /* Weather icon recolor on light theme; metric icons unchanged. */
    apply_icon_recolor(weather_img);
    apply_logo_recolor(logo_img);
    apply_logo_recolor(freenove_img);
    if (temp_img) {
        lv_obj_set_style_image_recolor_opa(temp_img, LV_OPA_TRANSP, 0);
    }
    if (hum_img) {
        lv_obj_set_style_image_recolor_opa(hum_img, LV_OPA_TRANSP, 0);
    }
}

static void ensure_reconfig_hint_widgets(void) {
    if (reconfig_panel != nullptr) {
        return;
    }
    if (scr == nullptr) {
        scr = lv_screen_active();
    }
    pick_fonts();

    reconfig_panel = lv_obj_create(scr);
    lv_obj_set_size(reconfig_panel, 240, 240);
    lv_obj_set_style_bg_color(reconfig_panel, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_bg_opa(reconfig_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(reconfig_panel, 0, 0);
    lv_obj_set_style_radius(reconfig_panel, 0, 0);
    lv_obj_set_style_pad_all(reconfig_panel, 0, 0);
    lv_obj_clear_flag(reconfig_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(reconfig_panel, LV_OBJ_FLAG_HIDDEN);

    const lv_font_t *countdown_font = font_clock_lg ? font_clock_lg : font_xl;
    reconfig_countdown = make_label(reconfig_panel, countdown_font, 0xFFFFFF);
    lv_obj_set_style_text_align(reconfig_countdown, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(reconfig_countdown, "10");
    lv_obj_align(reconfig_countdown, LV_ALIGN_TOP_MID, 0, 36);

    reconfig_label = make_label(reconfig_panel, font_md, 0xFFFFFF);
    lv_obj_set_style_text_align(reconfig_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(reconfig_label, 200);
    lv_label_set_long_mode(reconfig_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(reconfig_label,
                      "Warning!\nHold to reset WiFi setup!\n\nRelease to cancel");
    lv_obj_align(reconfig_label, LV_ALIGN_CENTER, 0, 28);
}

static void apply_theme_now(void) {
    if (scr == nullptr) {
        scr = lv_screen_active();
    }
    style_screen(scr);
    refresh_boot_theme_colors();
    refresh_main_theme_colors();
    refresh_analog_theme_colors();
    refresh_xl_theme_colors();
    lv_timer_handler();
}

void ui_clock_show_reconfig_hint(bool show) {
    ensure_reconfig_hint_widgets();
    if (show) {
        lv_obj_set_style_bg_color(reconfig_panel, lv_color_hex(0xFF0000), 0);
        lv_obj_set_style_bg_opa(reconfig_panel, LV_OPA_COVER, 0);
        lv_obj_set_style_text_color(reconfig_countdown, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_color(reconfig_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_clear_flag(reconfig_countdown, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(reconfig_label, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(reconfig_label,
                          "Warning!\nHold to reset WiFi setup!\n\nRelease to cancel");
        lv_obj_align(reconfig_label, LV_ALIGN_CENTER, 0, 28);
        lv_obj_align(reconfig_countdown, LV_ALIGN_TOP_MID, 0, 36);
        lv_obj_clear_flag(reconfig_panel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(reconfig_panel);
    } else {
        lv_obj_add_flag(reconfig_panel, LV_OBJ_FLAG_HIDDEN);
    }
    lv_timer_handler();
}

void ui_clock_set_reconfig_countdown(int seconds) {
    ensure_reconfig_hint_widgets();
    if (seconds < 0) {
        seconds = 0;
    }
    if (seconds > 10) {
        seconds = 10;
    }
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", seconds);
    lv_label_set_text(reconfig_countdown, buf);
    lv_obj_align(reconfig_countdown, LV_ALIGN_TOP_MID, 0, 36);
}

void ui_clock_show_blackout(const char *text) {
    ensure_reconfig_hint_widgets();
    lv_obj_set_style_bg_color(reconfig_panel, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(reconfig_panel, LV_OPA_COVER, 0);
    lv_obj_add_flag(reconfig_countdown, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_color(reconfig_label, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(reconfig_label, text ? text : "");
    lv_obj_align(reconfig_label, LV_ALIGN_CENTER, 0, 0);
    if (text && text[0] != '\0') {
        lv_obj_clear_flag(reconfig_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(reconfig_label, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_clear_flag(reconfig_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(reconfig_panel);
    lv_timer_handler();
    lv_timer_handler();
}

void ui_clock_set_theme(uint8_t theme) {
    s_theme = (theme == THEME_LIGHT) ? THEME_LIGHT : THEME_DARK;
    apply_theme_now();
}

uint8_t ui_clock_get_theme(void) {
    return s_theme;
}

uint8_t ui_clock_toggle_theme(void) {
    ui_clock_set_theme(s_theme == THEME_LIGHT ? THEME_DARK : THEME_LIGHT);
    return s_theme;
}

void ui_clock_set_face(uint8_t face) {
    if (face > UI_FACE_XL) {
        face = UI_FACE_WEATHER;
    }
    s_ui_face = face;
    if (city_label != nullptr || analog_root != nullptr || xl_root != nullptr) {
        apply_face_visibility();
        lv_timer_handler();
    }
}

uint8_t ui_clock_get_face(void) {
    return s_ui_face;
}

uint8_t ui_clock_toggle_face(void) {
    uint8_t next = (uint8_t)((s_ui_face + 1) % 3);
    ui_clock_set_face(next);
    return s_ui_face;
}

static void boot_label_slide_exec(void *obj, int32_t v) {
    lv_obj_align((lv_obj_t *)obj, LV_ALIGN_TOP_MID, (int32_t)v, 110);
}

void ui_clock_show_boot_page(BootPage page, const char *detail) {
    if (page >= BOOT_PAGE_COUNT) {
        page = BOOT_PAGE_READY;
    }

    /* Never leave the reconfig/blackout panel covering the classic boot UI. */
    if (reconfig_panel != nullptr) {
        lv_obj_add_flag(reconfig_panel, LV_OBJ_FLAG_HIDDEN);
    }

    ensure_boot_widgets();
    s_boot_page = page;

    lv_label_set_text(boot_title, boot_page_title(page));
    lv_label_set_text(boot_subtitle, boot_page_subtitle(page));
    lv_label_set_text(boot_label, detail ? detail : "");
    lv_obj_set_style_text_font(boot_label, font_sm, 0);
    lv_obj_set_width(boot_label, 220);
    lv_obj_set_height(boot_label, 92);
    lv_obj_set_style_text_line_space(boot_label, 6, 0);
    lv_obj_align(boot_label, LV_ALIGN_TOP_MID, 0, 110);

    lv_bar_set_value(boot_bar, boot_page_progress(page), LV_ANIM_OFF);

    lv_anim_t slide;
    lv_anim_init(&slide);
    lv_anim_set_var(&slide, boot_label);
    lv_anim_set_values(&slide, 24, 0);
    lv_anim_set_duration(&slide, 180);
    lv_anim_set_path_cb(&slide, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&slide, boot_label_slide_exec);
    lv_anim_start(&slide);

    refresh_boot_theme_colors();
    set_boot_visible(true);

    lv_timer_handler();
    lv_timer_handler();
}

void ui_clock_show_boot(const char *text) {
    ensure_boot_widgets();
    if (reconfig_panel != nullptr) {
        lv_obj_add_flag(reconfig_panel, LV_OBJ_FLAG_HIDDEN);
    }
    if (!s_boot_visible) {
        set_boot_visible(true);
    }
    lv_label_set_text(boot_label, text ? text : "");
    lv_obj_set_style_text_font(boot_label, font_sm, 0);
    lv_obj_set_width(boot_label, 220);
    lv_obj_set_height(boot_label, 92);
    lv_obj_set_style_text_line_space(boot_label, 6, 0);
    lv_obj_align(boot_label, LV_ALIGN_TOP_MID, 0, 110);
    boot_raise_chrome();
    lv_timer_handler();
    lv_timer_handler();
}

void ui_clock_create(void) {
    pick_fonts();
    scr = lv_screen_active();
    style_screen(scr);

    set_boot_visible(false);

    s_ui_face = config_store_get().ui_face;
    if (s_ui_face > UI_FACE_XL) {
        s_ui_face = UI_FACE_WEATHER;
    }

    if (city_label != nullptr) {
        if (analog_root == nullptr) {
            create_analog_face();
        }
        if (xl_root == nullptr) {
            create_xl_face();
        }
        apply_face_visibility();
        apply_theme_now();
        return;
    }

    const ThemePalette *p = pal();

    city_label = make_label(scr, font_md, p->text);
    lv_label_set_text(city_label, "City: --");
    lv_obj_set_pos(city_label, 8, 6);

    weather_img = lv_image_create(scr);
    lv_image_set_src(weather_img, &icon_weather_partly);
    lv_obj_align(weather_img, LV_ALIGN_TOP_RIGHT, -14, 2);

    cond_label = make_label(scr, font_sm, p->text);
    lv_label_set_text(cond_label, "Weather: --");
    lv_obj_set_pos(cond_label, 8, 30);

    hour_label = make_label(scr, font_clock_lg, p->hour);
    lv_label_set_text(hour_label, "00");
    lv_obj_set_pos(hour_label, 3, 62);
    scale_hm_label(hour_label, 300);

    colon_label = make_label(scr, font_clock_lg, p->text);
    lv_label_set_text(colon_label, ":");
    lv_obj_set_pos(colon_label, 89, 62);
    scale_hm_label(colon_label, 300);

    min_label = make_label(scr, font_clock_lg, p->min);
    lv_label_set_text(min_label, "00");
    lv_obj_set_pos(min_label, 104, 62);
    scale_hm_label(min_label, 300);

    ampm_box = lv_obj_create(scr);
    lv_obj_set_size(ampm_box, 36, 18);
    lv_obj_set_style_radius(ampm_box, 4, 0);
    lv_obj_set_style_bg_color(ampm_box, lv_color_hex(p->min), 0);
    lv_obj_set_style_bg_opa(ampm_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(ampm_box, 0, 0);
    lv_obj_set_style_pad_all(ampm_box, 0, 0);
    lv_obj_clear_flag(ampm_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(ampm_box, 188, 64);

    ampm_label = make_label(ampm_box, font_xs, p->ampm_fg);
    lv_label_set_text(ampm_label, "AM");
    lv_obj_center(ampm_label);

    sec_label = make_label(scr, font_clock_sm, p->hour);
    lv_label_set_text(sec_label, "00");
    lv_obj_set_pos(sec_label, 188, 90);

    date_label = make_label(scr, font_sm, p->text);
    lv_label_set_text(date_label, "--- -- ----  ---");
    lv_obj_set_pos(date_label, 10, 132);

    make_bar_row(174, &icon_temp, p->temp_bar, &temp_img, &temp_bar, &temp_value);
    make_bar_row(204, &icon_humidity, p->hum_bar, &hum_img, &hum_bar, &hum_value);

    logo_img = lv_image_create(scr);
    lv_image_set_src(logo_img, &icon_logo);
    lv_image_set_pivot(logo_img, UI_LOGO_W / 2, UI_LOGO_H / 2);
    lv_image_set_scale(logo_img, 130);
    lv_obj_align(logo_img, LV_ALIGN_BOTTOM_RIGHT, -12, -25);

    lv_anim_t logo_anim;
    lv_anim_init(&logo_anim);
    lv_anim_set_var(&logo_anim, logo_img);
    lv_anim_set_values(&logo_anim, 0, 3600);
    lv_anim_set_duration(&logo_anim, 4000);
    lv_anim_set_repeat_count(&logo_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&logo_anim, lv_anim_path_linear);
    lv_anim_set_exec_cb(&logo_anim, logo_rotate_exec);
    lv_anim_start(&logo_anim);

    freenove_img = lv_image_create(scr);
    lv_image_set_src(freenove_img, &icon_freenove);
    lv_obj_align(freenove_img, LV_ALIGN_BOTTOM_RIGHT, -8, -22);

    apply_icon_recolor(weather_img);
    apply_logo_recolor(logo_img);
    apply_logo_recolor(freenove_img);

    create_analog_face();
    create_xl_face();
    apply_face_visibility();

    lv_timer_handler();
}

/* Format month */
static void format_month_token(char *out, size_t out_len, int tm_mon, uint8_t month_en) {
    if (month_en == MONTH_ENG) {
        snprintf(out, out_len, "%s", month_name(tm_mon));
    } else {
        snprintf(out, out_len, "%02d", tm_mon + 1);
    }
}

/** Build date string from EEPROM display preferences. **/
static void format_date_line(char *out, size_t out_len, const struct tm &t,
                             uint8_t date_order, uint8_t month_en) {
    char month_tok[8];
    format_month_token(month_tok, sizeof(month_tok), t.tm_mon, month_en);
    const int year = t.tm_year + 1900;
    const int day = t.tm_mday;
    const char *wday = wday_name(t.tm_wday);
    const char *sep = (month_en == MONTH_ENG) ? " " : "-";

    switch (date_order) {
        case DATE_ORDER_MDY:
            if (month_en == MONTH_ENG) {
                snprintf(out, out_len, "%s %d %d  %s", month_tok, day, year, wday);
            } else {
                snprintf(out, out_len, "%s%s%02d%s%d  %s",
                         month_tok, sep, day, sep, year, wday);
            }
            break;
        case DATE_ORDER_DMY:
            if (month_en == MONTH_ENG) {
                snprintf(out, out_len, "%d %s %d  %s", day, month_tok, year, wday);
            } else {
                snprintf(out, out_len, "%02d%s%s%s%d  %s",
                         day, sep, month_tok, sep, year, wday);
            }
            break;
        case DATE_ORDER_YMD:
        default:
            if (month_en == MONTH_ENG) {
                snprintf(out, out_len, "%d %s %d  %s", year, month_tok, day, wday);
            } else {
                snprintf(out, out_len, "%d%s%s%s%02d  %s",
                         year, sep, month_tok, sep, day, wday);
            }
            break;
    }
}

void ui_clock_update_time(void) {
    struct tm t;
    const bool have_time = ntp_time_get(&t);

#if UI_CLOCK_FORCE_0000
    if (hour_label != nullptr) {
        lv_label_set_text(hour_label, "00");
        lv_label_set_text(min_label, "00");
        if (colon_label != nullptr) {
            lv_label_set_text(colon_label, ":");
        }
    }
#endif

    if (!have_time) {
        return;
    }

    if (analog_root != nullptr) {
        update_analog_hands(t);
    }
    if (xl_root != nullptr) {
        update_xl_time(t);
    }

    if (hour_label == nullptr) return;

    const AppConfig &cfg = config_store_get();
    const bool use_12h = (cfg.hour12 == HOUR_FMT_12);

#if !UI_CLOCK_FORCE_0000
    int display_hour = t.tm_hour;
    if (use_12h) {
        display_hour %= 12;
        if (display_hour == 0) {
            display_hour = 12;
        }
    }

    char hm_buf[8];
    snprintf(hm_buf, sizeof(hm_buf), "%02d", display_hour);
    lv_label_set_text(hour_label, hm_buf);

    snprintf(hm_buf, sizeof(hm_buf), "%02d", t.tm_min);
    lv_label_set_text(min_label, hm_buf);
#endif

    char buf[8];
    snprintf(buf, sizeof(buf), "%02d", t.tm_sec);
    lv_label_set_text(sec_label, buf);

    lv_label_set_text(ampm_label, (t.tm_hour >= 12) ? "PM" : "AM");
    lv_obj_center(ampm_label);

    lv_obj_set_style_opa(ampm_label, use_12h ? LV_OPA_COVER : LV_OPA_70, 0);

    char date_buf[40];
    format_date_line(date_buf, sizeof(date_buf), t, cfg.date_order, cfg.month_en);
    lv_label_set_text(date_label, date_buf);
}

void ui_clock_update_weather(const WeatherInfo &w) {
    if (city_label == nullptr) return;

    const char *raw = w.resolved_name[0] ? w.resolved_name : w.query_city;
    char city[48];
    strncpy(city, raw ? raw : "", sizeof(city) - 1);
    city[sizeof(city) - 1] = '\0';

    char *paren = strchr(city, '(');
    if (paren) {
        *paren = '\0';
        size_t n = strlen(city);
        while (n > 0 && city[n - 1] == ' ') {
            city[--n] = '\0';
        }
    }

    char city_line[56];
    snprintf(city_line, sizeof(city_line), "%s", city[0] ? city : "--");
    lv_label_set_text(city_label, city_line);

    char cond[56];
    snprintf(cond, sizeof(cond), "%s", w.condition[0] ? w.condition : "--");
    lv_label_set_text(cond_label, cond);

    set_weather_icon(w.weather_code);

    int temp_pct = (int)(w.temperature_c * 100.0f / 45.0f);
    if (temp_pct < 0) temp_pct = 0;
    if (temp_pct > 100) temp_pct = 100;
    lv_bar_set_value(temp_bar, temp_pct, LV_ANIM_OFF);
    lv_bar_set_value(hum_bar, constrain(w.humidity_pct, 0, 100), LV_ANIM_OFF);

    const AppConfig &cfg = config_store_get();
    char buf[16];
    if (cfg.temp_unit == TEMP_UNIT_F) {
        float f = w.temperature_c * 9.0f / 5.0f + 32.0f;
        snprintf(buf, sizeof(buf), "%.0fF", f);
    } else {
        snprintf(buf, sizeof(buf), "%.0fC", w.temperature_c);
    }
    lv_label_set_text(temp_value, buf);
    snprintf(buf, sizeof(buf), "%d%%", w.humidity_pct);
    lv_label_set_text(hum_value, buf);

    update_analog_weather(w);
}
