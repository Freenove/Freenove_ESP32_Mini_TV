#include "wifi_setup.h"
#include "debug_log.h"
#include "config_store.h"
#include "ntp_time.h"

#include <WiFi.h>
#include <WiFiManager.h>
#include <stdio.h>
#include <string.h>

static void notify(WifiStatusCallback cb, const char *text) {
    DBG_PRINTLN(text);
    if (cb) {
        cb(text);
    }
}

/** Attempt STA association with EEPROM credentials. **/
static bool try_connect_saved(WifiStatusCallback on_status) {
    if (!config_store_is_valid()) {
        return false;
    }

    const AppConfig &cfg = config_store_get();
    char msg[96];
    snprintf(msg, sizeof(msg), "Connecting WiFi...\n%s",
             cfg.ssid[0] ? cfg.ssid : "saved network");
    notify(on_status, msg);

    WiFi.mode(WIFI_STA);
    WiFi.begin(cfg.ssid, cfg.password);

    const uint32_t deadline = millis() + (uint32_t)WIFI_CONNECT_TIMEOUT_S * 1000UL;
    uint32_t last_ui = 0;
    int dots = 0;

    while (WiFi.status() != WL_CONNECTED && millis() < deadline) {
        delay(200);
        DBG_PRINT('.');

        if (millis() - last_ui >= 800) {
            last_ui = millis();
            dots = (dots + 1) % 4;
            char wait[96];
            snprintf(wait, sizeof(wait), "Connecting WiFi...\n%s\n%.*s",
                     cfg.ssid[0] ? cfg.ssid : "saved network",
                     dots + 1, "...");
            notify(on_status, wait);
        }
    }
    DBG_PRINTLN();

    return WiFi.status() == WL_CONNECTED;
}

static char s_html_hour[280];
static char s_html_date[360];
static char s_html_month[280];
static WiFiManager *s_portal_wm = nullptr;
static uint8_t s_portal_hour12 = HOUR_FMT_24;
static uint8_t s_portal_date_order = DATE_ORDER_DMY;
static uint8_t s_portal_month_en = MONTH_NUM;

static void build_display_format_html(const AppConfig &cfg) {
    snprintf(s_html_hour, sizeof(s_html_hour),
             "<br/><label for='hour12'>Clock format</label><br/>"
             "<select name='hour12' id='hour12'>"
             "<option value='0'%s>24-hour</option>"
             "<option value='1'%s>12-hour (AM/PM)</option>"
             "</select>",
             cfg.hour12 == HOUR_FMT_12 ? "" : " selected",
             cfg.hour12 == HOUR_FMT_12 ? " selected" : "");

    snprintf(s_html_date, sizeof(s_html_date),
             "<br/><label for='date_order'>Date order</label><br/>"
             "<select name='date_order' id='date_order'>"
             "<option value='0'%s>Year-Month-Day</option>"
             "<option value='1'%s>Month-Day-Year</option>"
             "<option value='2'%s>Day-Month-Year</option>"
             "</select>",
             cfg.date_order == DATE_ORDER_YMD ? " selected" : "",
             cfg.date_order == DATE_ORDER_MDY ? " selected" : "",
             cfg.date_order == DATE_ORDER_DMY ? " selected" : "");

    snprintf(s_html_month, sizeof(s_html_month),
             "<br/><label for='month_en'>Month style</label><br/>"
             "<select name='month_en' id='month_en'>"
             "<option value='0'%s>Number (01-12)</option>"
             "<option value='1'%s>English (Jan-Dec)</option>"
             "</select>",
             cfg.month_en == MONTH_ENG ? "" : " selected",
             cfg.month_en == MONTH_ENG ? " selected" : "");
}

static void on_portal_save_params(void) {
    if (s_portal_wm == nullptr || s_portal_wm->server == nullptr) {
        return;
    }
    if (s_portal_wm->server->hasArg("hour12")) {
        int v = s_portal_wm->server->arg("hour12").toInt();
        s_portal_hour12 = (v == HOUR_FMT_12) ? HOUR_FMT_12 : HOUR_FMT_24;
    }
    if (s_portal_wm->server->hasArg("date_order")) {
        int v = s_portal_wm->server->arg("date_order").toInt();
        if (v < DATE_ORDER_YMD || v > DATE_ORDER_DMY) {
            v = DATE_ORDER_DMY;
        }
        s_portal_date_order = (uint8_t)v;
    }
    if (s_portal_wm->server->hasArg("month_en")) {
        int v = s_portal_wm->server->arg("month_en").toInt();
        s_portal_month_en = (v == MONTH_ENG) ? MONTH_ENG : MONTH_NUM;
    }
}

/** SoftAP portal: SSID/password, UTC offset, clock/date display format. */
static bool start_config_portal(WifiStatusCallback on_status) {
    char portal_msg[96];
    snprintf(portal_msg, sizeof(portal_msg),
             "Open config portal\nSSID: %s\nConnect phone WiFi", WIFI_AP_NAME);
    notify(on_status, portal_msg);

    const AppConfig &cfg = config_store_get();

    char utc_offset_buf[8];
    snprintf(utc_offset_buf, sizeof(utc_offset_buf), "%d",
             ntp_timezone_utc_offset(cfg.timezone[0] ? cfg.timezone : nullptr));

    s_portal_hour12 = (cfg.hour12 == HOUR_FMT_12) ? HOUR_FMT_12 : HOUR_FMT_24;
    s_portal_date_order = (cfg.date_order <= DATE_ORDER_DMY) ? cfg.date_order : DATE_ORDER_DMY;
    s_portal_month_en = (cfg.month_en == MONTH_ENG) ? MONTH_ENG : MONTH_NUM;
    build_display_format_html(cfg);

    WiFiManager wm;
    s_portal_wm = &wm;
    wm.setDebugOutput(true);
    wm.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT_S);
    /* Capture custom <select> values from the portal POST body. */
    wm.setSaveParamsCallback(on_portal_save_params);
    wm.setSaveConfigCallback(on_portal_save_params);

    WiFiManagerParameter custom_tz(
        "timezone",
        "UTC offset hours (e.g., 8)",
        utc_offset_buf,
        sizeof(utc_offset_buf) - 1
    );
    WiFiManagerParameter custom_hour(s_html_hour);
    WiFiManagerParameter custom_date(s_html_date);
    WiFiManagerParameter custom_month(s_html_month);

    wm.addParameter(&custom_tz);
    wm.addParameter(&custom_hour);
    wm.addParameter(&custom_date);
    wm.addParameter(&custom_month);

    const char *ap_pass = (WIFI_AP_PASSWORD[0] != '\0') ? WIFI_AP_PASSWORD : nullptr;
    bool ok = wm.startConfigPortal(WIFI_AP_NAME, ap_pass);
    s_portal_wm = nullptr;
    if (!ok) {
        return false;
    }

    const char *tz_raw = custom_tz.getValue();
    char tz_posix[24];
    ntp_timezone_normalize(tz_raw, tz_posix, sizeof(tz_posix));

    String ssid = WiFi.SSID();
    String pass = WiFi.psk();
    config_store_save(ssid.c_str(), pass.c_str(), tz_posix, cfg.city);
    config_store_save_display_format(s_portal_hour12, s_portal_date_order, s_portal_month_en);
    DBG_PRINTF("[WIFI] Saved timezone input=\"%s\" -> %s (UTC%+d)\n",
                  tz_raw ? tz_raw : "", tz_posix, ntp_timezone_utc_offset(tz_posix));
    DBG_PRINTF("[WIFI] Display: clock=%s date_order=%u month=%s\n",
                  s_portal_hour12 == HOUR_FMT_12 ? "12h" : "24h",
                  (unsigned)s_portal_date_order,
                  s_portal_month_en == MONTH_ENG ? "Eng" : "Num");
    return true;
}

static void show_connected(WifiStatusCallback on_status) {
    const AppConfig &cfg = config_store_get();
    DBG_PRINT("WiFi connected. IP: ");
    DBG_PRINTLN(WiFi.localIP());
    DBG_PRINTF("Timezone: %s (UTC%+d)  CachedCity: %s\n",
                  cfg.timezone, ntp_timezone_utc_offset(cfg.timezone),
                  cfg.city[0] ? cfg.city : "(auto)");

    if (on_status) {
        String msg = "WiFi OK\n";
        msg += WiFi.SSID();
        msg += "\n";
        msg += WiFi.localIP().toString();
        on_status(msg.c_str());
    }
}

bool wifi_setup_begin(WifiStatusCallback on_status) {
    config_store_begin();
    WiFi.mode(WIFI_STA);

    if (try_connect_saved(on_status)) {
        show_connected(on_status);
        return true;
    }

    DBG_PRINTLN("Saved WiFi failed / empty, starting portal...");
    if (!start_config_portal(on_status)) {
        notify(on_status, "WiFi Failed\nReset to retry");
        return false;
    }

    show_connected(on_status);
    return true;
}

void wifi_setup_reset(void) {
    config_store_clear();
    WiFiManager wm;
    wm.resetSettings();
    DBG_PRINTLN("WiFi credentials cleared");
}
