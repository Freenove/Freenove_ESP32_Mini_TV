#include "config_store.h"
#include "debug_log.h"

#include <EEPROM.h>
#include <math.h>
#include <string.h>

#include "ntp_time.h"

#define CONFIG_EEPROM_SIZE  sizeof(AppConfig)

static AppConfig g_cfg;

static void clamp_display_format(AppConfig &cfg) {
    if (cfg.hour12 != HOUR_FMT_12) {
        cfg.hour12 = HOUR_FMT_24;
    }
    if (cfg.date_order > DATE_ORDER_DMY) {
        cfg.date_order = DATE_ORDER_DMY;
    }
    if (cfg.month_en != MONTH_ENG) {
        cfg.month_en = MONTH_NUM;
    }
    if (cfg.ui_face > UI_FACE_XL) {
        cfg.ui_face = UI_FACE_WEATHER;
    }
}

static void config_set_defaults(AppConfig &cfg) {
    memset(&cfg, 0, sizeof(cfg));
    cfg.magic = 0;
    strncpy(cfg.timezone, CONFIG_DEFAULT_TZ, CONFIG_TZ_LEN - 1);
    cfg.theme = THEME_DARK;
    cfg.hour12 = HOUR_FMT_24;
    cfg.date_order = DATE_ORDER_DMY;
    cfg.month_en = MONTH_NUM;
    cfg.ui_face = UI_FACE_WEATHER;
}

static bool coords_look_valid(float lat, float lon, uint8_t flag) {
    if (flag != 1) {
        return false;
    }
    if (isnan(lat) || isnan(lon)) {
        return false;
    }
    if (lat < -90.0f || lat > 90.0f || lon < -180.0f || lon > 180.0f) {
        return false;
    }
    if (fabsf(lat) < 0.0001f && fabsf(lon) < 0.0001f) {
        return false;
    }
    return true;
}

void config_store_begin(void) {
    EEPROM.begin(CONFIG_EEPROM_SIZE);
    config_set_defaults(g_cfg);
    EEPROM.get(0, g_cfg);

    if (g_cfg.magic != CONFIG_MAGIC) {
        config_set_defaults(g_cfg);
        DBG_PRINTLN("[CFG] No valid EEPROM config");
        return;
    }

    g_cfg.ssid[CONFIG_SSID_LEN - 1] = '\0';
    g_cfg.password[CONFIG_PASS_LEN - 1] = '\0';
    g_cfg.timezone[CONFIG_TZ_LEN - 1] = '\0';
    g_cfg.city[CONFIG_CITY_LEN - 1] = '\0';

    if (g_cfg.timezone[0] == '\0') {
        strncpy(g_cfg.timezone, CONFIG_DEFAULT_TZ, CONFIG_TZ_LEN - 1);
    }
    if (!coords_look_valid(g_cfg.latitude, g_cfg.longitude, g_cfg.has_coords)) {
        g_cfg.has_coords = 0;
        g_cfg.latitude = 0;
        g_cfg.longitude = 0;
    }
    if (g_cfg.theme != THEME_LIGHT) {
        g_cfg.theme = THEME_DARK;
    }
    clamp_display_format(g_cfg);

    DBG_PRINTF("[CFG] Loaded SSID=\"%s\" TZ=\"%s\" City=\"%s\" Theme=%s "
                  "Face=%s Clock=%s Date=%u Month=%s",
                  g_cfg.ssid, g_cfg.timezone, g_cfg.city[0] ? g_cfg.city : "(auto)",
                  g_cfg.theme == THEME_LIGHT ? "light" : "dark",
                  g_cfg.ui_face == UI_FACE_ANALOG ? "analog" :
                  (g_cfg.ui_face == UI_FACE_XL ? "xl" : "weather"),
                  g_cfg.hour12 == HOUR_FMT_12 ? "12h" : "24h",
                  (unsigned)g_cfg.date_order,
                  g_cfg.month_en == MONTH_ENG ? "Eng" : "Num");
    if (g_cfg.has_coords) {
        DBG_PRINTF(" @ %.4f,%.4f", g_cfg.latitude, g_cfg.longitude);
    }
    DBG_PRINTLN();
}

bool config_store_is_valid(void) {
    return g_cfg.magic == CONFIG_MAGIC && g_cfg.ssid[0] != '\0';
}

bool config_store_has_coords(void) {
    return coords_look_valid(g_cfg.latitude, g_cfg.longitude, g_cfg.has_coords);
}

const AppConfig &config_store_get(void) {
    return g_cfg;
}

bool config_store_save(const char *ssid, const char *password,
                       const char *timezone, const char *city) {
    if (ssid == nullptr || ssid[0] == '\0') {
        return false;
    }

    char city_keep[CONFIG_CITY_LEN];
    float lat_keep = g_cfg.latitude;
    float lon_keep = g_cfg.longitude;
    uint8_t coords_keep = g_cfg.has_coords;
    uint8_t theme_keep = g_cfg.theme;
    uint8_t hour12_keep = g_cfg.hour12;
    uint8_t date_order_keep = g_cfg.date_order;
    uint8_t month_en_keep = g_cfg.month_en;
    uint8_t ui_face_keep = g_cfg.ui_face;
    strncpy(city_keep, g_cfg.city, CONFIG_CITY_LEN - 1);
    city_keep[CONFIG_CITY_LEN - 1] = '\0';

    memset(&g_cfg, 0, sizeof(g_cfg));
    g_cfg.magic = CONFIG_MAGIC;
    strncpy(g_cfg.ssid, ssid, CONFIG_SSID_LEN - 1);
    if (password) {
        strncpy(g_cfg.password, password, CONFIG_PASS_LEN - 1);
    }
    if (timezone && timezone[0] != '\0') {
        char tz_norm[CONFIG_TZ_LEN];
        ntp_timezone_normalize(timezone, tz_norm, sizeof(tz_norm));
        strncpy(g_cfg.timezone, tz_norm, CONFIG_TZ_LEN - 1);
    } else {
        strncpy(g_cfg.timezone, CONFIG_DEFAULT_TZ, CONFIG_TZ_LEN - 1);
    }

    if (city && city[0] != '\0') {
        strncpy(g_cfg.city, city, CONFIG_CITY_LEN - 1);
    } else {
        strncpy(g_cfg.city, city_keep, CONFIG_CITY_LEN - 1);
    }
    g_cfg.latitude = lat_keep;
    g_cfg.longitude = lon_keep;
    g_cfg.has_coords = coords_keep;
    g_cfg.theme = (theme_keep == THEME_LIGHT) ? THEME_LIGHT : THEME_DARK;
    g_cfg.hour12 = hour12_keep;
    g_cfg.date_order = date_order_keep;
    g_cfg.month_en = month_en_keep;
    g_cfg.ui_face = ui_face_keep;
    clamp_display_format(g_cfg);

    EEPROM.put(0, g_cfg);
    bool ok = EEPROM.commit();
    DBG_PRINTF("[CFG] Saved SSID=\"%s\" TZ=\"%s\" City=\"%s\" (%s)\n",
                  g_cfg.ssid, g_cfg.timezone,
                  g_cfg.city[0] ? g_cfg.city : "(auto)",
                  ok ? "OK" : "FAIL");
    return ok;
}

bool config_store_save_timezone(const char *timezone) {
    if (!config_store_is_valid()) {
        return false;
    }
    if (timezone && timezone[0] != '\0') {
        char tz_norm[CONFIG_TZ_LEN];
        ntp_timezone_normalize(timezone, tz_norm, sizeof(tz_norm));
        strncpy(g_cfg.timezone, tz_norm, CONFIG_TZ_LEN - 1);
        g_cfg.timezone[CONFIG_TZ_LEN - 1] = '\0';
    }
    EEPROM.put(0, g_cfg);
    return EEPROM.commit();
}

bool config_store_save_location(const char *city, float latitude, float longitude) {
    if (!config_store_is_valid()) {
        return false;
    }
    if (city && city[0] != '\0') {
        strncpy(g_cfg.city, city, CONFIG_CITY_LEN - 1);
        g_cfg.city[CONFIG_CITY_LEN - 1] = '\0';
    }
    g_cfg.latitude = latitude;
    g_cfg.longitude = longitude;
    g_cfg.has_coords = coords_look_valid(latitude, longitude, 1) ? 1 : 0;

    EEPROM.put(0, g_cfg);
    bool ok = EEPROM.commit();
    DBG_PRINTF("[CFG] Location saved: %s (%.4f, %.4f) %s\n",
                  g_cfg.city, g_cfg.latitude, g_cfg.longitude, ok ? "OK" : "FAIL");
    return ok;
}

bool config_store_save_theme(uint8_t theme) {
    uint8_t t = (theme == THEME_LIGHT) ? THEME_LIGHT : THEME_DARK;

    if (g_cfg.magic != CONFIG_MAGIC) {
        char tz_keep[CONFIG_TZ_LEN];
        strncpy(tz_keep, g_cfg.timezone[0] ? g_cfg.timezone : CONFIG_DEFAULT_TZ,
                CONFIG_TZ_LEN - 1);
        tz_keep[CONFIG_TZ_LEN - 1] = '\0';
        config_set_defaults(g_cfg);
        g_cfg.magic = CONFIG_MAGIC;
        strncpy(g_cfg.timezone, tz_keep, CONFIG_TZ_LEN - 1);
    }

    g_cfg.theme = t;
    EEPROM.put(0, g_cfg);
    bool ok = EEPROM.commit();
    DBG_PRINTF("[CFG] Theme saved: %s (%s)\n",
                  t == THEME_LIGHT ? "light" : "dark", ok ? "OK" : "FAIL");
    return ok;
}

bool config_store_save_display_format(uint8_t hour12, uint8_t date_order, uint8_t month_en) {
    g_cfg.hour12 = hour12;
    g_cfg.date_order = date_order;
    g_cfg.month_en = month_en;
    clamp_display_format(g_cfg);
    if (g_cfg.magic != CONFIG_MAGIC) {
        g_cfg.magic = CONFIG_MAGIC;
    }

    EEPROM.put(0, g_cfg);
    bool ok = EEPROM.commit();
    DBG_PRINTF("[CFG] Display format: clock=%s date_order=%u month=%s (%s)\n",
                  g_cfg.hour12 == HOUR_FMT_12 ? "12h" : "24h",
                  (unsigned)g_cfg.date_order,
                  g_cfg.month_en == MONTH_ENG ? "Eng" : "Num",
                  ok ? "OK" : "FAIL");
    return ok;
}

bool config_store_save_ui_face(uint8_t ui_face) {
    if (ui_face > UI_FACE_XL) {
        ui_face = UI_FACE_WEATHER;
    }
    g_cfg.ui_face = ui_face;
    if (g_cfg.magic != CONFIG_MAGIC) {
        g_cfg.magic = CONFIG_MAGIC;
    }
    EEPROM.put(0, g_cfg);
    bool ok = EEPROM.commit();
    const char *name = "weather";
    if (g_cfg.ui_face == UI_FACE_ANALOG) {
        name = "analog";
    } else if (g_cfg.ui_face == UI_FACE_XL) {
        name = "xl";
    }
    DBG_PRINTF("[CFG] UI face saved: %s (%s)\n", name, ok ? "OK" : "FAIL");
    return ok;
}

void config_store_clear(void) {
    config_set_defaults(g_cfg);
    EEPROM.put(0, g_cfg);
    EEPROM.commit();
    DBG_PRINTLN("[CFG] EEPROM cleared");
}
