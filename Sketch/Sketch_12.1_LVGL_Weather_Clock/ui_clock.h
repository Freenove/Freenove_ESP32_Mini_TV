#ifndef UI_CLOCK_H
#define UI_CLOCK_H

#include <Arduino.h>
#include <lvgl.h>
#include "weather.h"
#include "config_store.h"

enum BootPage : uint8_t {
    BOOT_PAGE_START = 0,
    BOOT_PAGE_WIFI,
    BOOT_PAGE_NTP,
    BOOT_PAGE_WEATHER,
    BOOT_PAGE_READY,
    BOOT_PAGE_COUNT
};

/** Create main clock UI. Creates weather + analog faces. */
void ui_clock_create(void);

void ui_clock_show_boot_page(BootPage page, const char *detail);
void ui_clock_show_boot(const char *text);

void ui_clock_set_theme(uint8_t theme);
uint8_t ui_clock_get_theme(void);
uint8_t ui_clock_toggle_theme(void);

void ui_clock_set_face(uint8_t face);
uint8_t ui_clock_get_face(void);
uint8_t ui_clock_toggle_face(void);

void ui_clock_update_time(void);
void ui_clock_update_weather(const WeatherInfo &w);

/** Overlay while holding for WiFi reconfig (show at 6s, hide on release). */
void ui_clock_show_reconfig_hint(bool show);

/** Update digital countdown (10..0) on the reconfig warning screen. */
void ui_clock_set_reconfig_countdown(int seconds);

/** Full-screen black (optional white status text). Used before/during WiFi portal. */
void ui_clock_show_blackout(const char *text);

#endif
