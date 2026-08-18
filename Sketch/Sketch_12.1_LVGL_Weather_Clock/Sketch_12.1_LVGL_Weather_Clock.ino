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
static bool reconfig_hint_visible = false;
static bool reconfig_in_progress = false;

static void boot_status_live(const char *text) {
    ui_clock_show_boot(text);
}

static void on_theme_long_press(void);
static void on_face_long_press(void);
static void on_reconfig_hint(void);
static void on_reconfig_wifi(void);
static void update_reconfig_countdown(void);
static void hide_reconfig_hint_if_needed(void);

static void process_touch_events(void) {
    if (reconfig_in_progress) {
        return;
    }

    TouchEvent ev = touch_button_poll();
    if (ev == TOUCH_EVENT_RECONFIG_HINT) {
        on_reconfig_hint();
    } else if (ev == TOUCH_EVENT_RECONFIG) {
        on_reconfig_wifi();
    } else if (ui_ready) {
        if (ev == TOUCH_EVENT_THEME) {
            on_theme_long_press();
        } else if (ev == TOUCH_EVENT_FACE) {
            on_face_long_press();
        }
    }
    update_reconfig_countdown();
    hide_reconfig_hint_if_needed();
}

static void wifi_idle_pump(void) {
    lvgl_driver_handler();
    process_touch_events();
    delay(5);
}

/** Show a boot page **/
static void boot_page(BootPage page, const char *detail, uint32_t hold_ms = 450) {
    ui_clock_show_boot_page(page, detail);
    uint32_t start = millis();
    while (millis() - start < hold_ms) {
        if (reconfig_in_progress) {
            return;
        }
        wifi_idle_pump();
    }
}

static void pump_display(uint32_t ms) {
    uint32_t start = millis();
    while (millis() - start < ms) {
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

static int reconfig_remaining_sec(void) {
    uint32_t held = touch_button_held_ms();
    if (held < TOUCH_RECONFIG_HINT_MS) {
        return 10;
    }

    int rem = 10 - (int)((held - TOUCH_RECONFIG_HINT_MS) / 1000UL);
    if (rem < 0) {
        rem = 0;
    }
    if (rem > 10) {
        rem = 10;
    }
    return rem;
}

static void on_reconfig_hint(void) {
    reconfig_hint_visible = true;
    ui_clock_show_reconfig_hint(true);
    ui_clock_set_reconfig_countdown(reconfig_remaining_sec());
}

static void hide_reconfig_hint_if_needed(void) {
    if (reconfig_hint_visible && !touch_button_is_pressed()) {
        reconfig_hint_visible = false;
        ui_clock_show_reconfig_hint(false);
    }
}

static void update_reconfig_countdown(void) {
    if (!reconfig_hint_visible || !touch_button_is_pressed()) {
        return;
    }
    ui_clock_set_reconfig_countdown(reconfig_remaining_sec());
}

/** Clear saved WiFi and reboot into SoftAP portal. */
static void on_reconfig_wifi(void) {
    if (reconfig_in_progress) {
        return;
    }
    reconfig_in_progress = true;
    reconfig_hint_visible = false;

    ui_clock_show_blackout("Restarting...");
    pump_display(80);
    wifi_setup_reset();
    pump_display(80);
    ESP.restart();
}

void setup() {
    DBG_BEGIN(115200);
    delay(200);

    lvgl_driver_init();
    touch_button_begin();
    config_store_begin();
    ui_clock_set_theme(config_store_get().theme);

    if (!config_store_is_valid()) {
        ui_clock_set_theme(THEME_DARK);
        boot_page(BOOT_PAGE_WIFI,
                  "Use your phone to join\nthe following Wi-Fi:\n"
                  "Name: " WIFI_AP_NAME "\nNo password",
                  300);
    } else {
        boot_page(BOOT_PAGE_START, "Weather Clock\nStarting system...", 500);
        DBG_PRINTLN("System Ready");
        boot_page(BOOT_PAGE_WIFI, "Preparing WiFi...", 350);
    }

    if (!wifi_setup_begin(boot_status_live, wifi_idle_pump)) {
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
        const AppConfig &cfg = config_store_get();
        char msg[96];
        if (cfg.temp_unit == TEMP_UNIT_F) {
            float f = w.temperature_c * 9.0f / 5.0f + 32.0f;
            snprintf(msg, sizeof(msg), "Got weather\n%s  %.0fF",
                     w.resolved_name[0] ? w.resolved_name : "OK", f);
        } else {
            snprintf(msg, sizeof(msg), "Got weather\n%s  %.0fC",
                     w.resolved_name[0] ? w.resolved_name : "OK",
                     w.temperature_c);
        }
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
    process_touch_events();

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
