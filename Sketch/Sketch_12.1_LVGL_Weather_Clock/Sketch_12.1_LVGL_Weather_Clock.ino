/*
* @ File:   Sketch_12.1_LVGL_Weather_Clock.ino
* @ Author: [Vegetable-SYC]
* @ Date:   [2026-08-07]
*/

#include "debug_log.h"
#include "lvgl_driver.h"
#include "wifi_setup.h"
#include "config_store.h"
#include "ntp_time.h"
#include "weather.h"
#include "ui_clock.h"
#include "touch_button.h"

#ifndef WEATHER_REFRESH_MS
#define WEATHER_REFRESH_MS (10UL * 60UL * 1000UL)
#endif

static uint32_t last_time_ms = 0;
static uint32_t last_weather_ms = 0;
static bool ui_ready = false;

static void boot_status_live(const char *text) {
    ui_clock_show_boot(text);
}

/** Show a boot page **/
static void boot_page(BootPage page, const char *detail, uint32_t hold_ms = 450) {
    ui_clock_show_boot_page(page, detail);
    uint32_t start = millis();
    while (millis() - start < hold_ms) {
        lvgl_driver_handler();
        delay(5);
    }
}

/** Fetch weather **/
static bool refresh_weather(void) {
    if (!ui_ready) {
        boot_page(BOOT_PAGE_WEATHER, "Locating city\nby public IP...", 300);
    }

    bool ok = weather_update_auto();

    if (ok && ui_ready) {
        ui_clock_update_weather(weather_get());
    } else if (!ok && !ui_ready) {
        boot_page(BOOT_PAGE_WEATHER, "Weather failed\nWill retry later", 600);
    }

    last_weather_ms = millis();
    return ok;
}

static void on_theme_long_press(void) {
    uint8_t next = ui_clock_toggle_theme();
    config_store_save_theme(next);
    DBG_PRINTF("[UI] Theme -> %s\n", next == THEME_LIGHT ? "light" : "dark");
}

static void on_face_long_press(void) {
    uint8_t next = ui_clock_toggle_face();
    config_store_save_ui_face(next);
    const char *name = "weather";
    if (next == UI_FACE_ANALOG) {
        name = "analog";
    } else if (next == UI_FACE_XL) {
        name = "xl";
    }
    DBG_PRINTF("[UI] Face -> %s\n", name);
}

void setup() {
    DBG_BEGIN(115200);
    delay(200);

    lvgl_driver_init();
    touch_button_begin();
    config_store_begin();
    ui_clock_set_theme(config_store_get().theme);

    boot_page(BOOT_PAGE_START, "Weather Clock\nStarting system...", 500);
    DBG_PRINTLN("System Ready");

    boot_page(BOOT_PAGE_WIFI, "Preparing WiFi...", 350);
    if (!wifi_setup_begin(boot_status_live)) {
        boot_page(BOOT_PAGE_WIFI, "WiFi setup failed\nReset to retry", 0);
        return;
    }
    boot_page(BOOT_PAGE_WIFI, "WiFi connected", 400);

    boot_page(BOOT_PAGE_NTP, "Syncing time\nvia NTP...", 350);
    if (!ntp_time_begin(config_store_get().timezone)) {
        boot_page(BOOT_PAGE_NTP, "NTP sync failed\nClock may be wrong", 800);
    } else {
        boot_page(BOOT_PAGE_NTP, "Time synced OK", 400);
    }

    boot_page(BOOT_PAGE_WEATHER, "Fetching weather...", 300);
    refresh_weather();
    if (weather_get().valid) {
        const WeatherInfo &w = weather_get();
        char msg[96];
        snprintf(msg, sizeof(msg), "Got weather\n%s  %.0fC",
                 w.resolved_name[0] ? w.resolved_name : "OK",
                 w.temperature_c);
        boot_page(BOOT_PAGE_WEATHER, msg, 500);
    }

    boot_page(BOOT_PAGE_READY, "Building UI...", 350);
    ui_clock_create();
    ui_clock_set_face(config_store_get().ui_face);
    ui_ready = true;
    ui_clock_update_time();
    if (weather_get().valid) {
        ui_clock_update_weather(weather_get());
    }
}

void loop() {
    lvgl_driver_handler();

    TouchEvent ev = touch_button_poll();
    if (ev == TOUCH_EVENT_THEME) {
        on_theme_long_press();
    } else if (ev == TOUCH_EVENT_FACE) {
        on_face_long_press();
    }

    uint32_t now = millis();

    if (ui_ready && (now - last_time_ms >= 1000)) {
        last_time_ms = now;
        ui_clock_update_time();
        ntp_time_print();
    }

    if (ui_ready && (now - last_weather_ms >= WEATHER_REFRESH_MS)) {
        refresh_weather();
    }

    delay(5);
}
