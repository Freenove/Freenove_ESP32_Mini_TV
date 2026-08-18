#include "weather.h"
#include "debug_log.h"
#include "config_store.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static WeatherInfo g_weather;

static void weather_clear(void) {
    memset(&g_weather, 0, sizeof(g_weather));
}

static String url_encode(const char *str) {
    String out;
    if (str == nullptr) {
        return out;
    }
    for (const uint8_t *p = (const uint8_t *)str; *p; ++p) {
        char c = (char)*p;
        if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += c;
        } else if (c == ' ') {
            out += "%20";
        } else {
            char hex[4];
            snprintf(hex, sizeof(hex), "%%%02X", *p);
            out += hex;
        }
    }
    return out;
}

/** Strip parenthetical suffix from city names for geocoding. */
static void city_basename(const char *in, char *out, size_t out_len) {
    if (out == nullptr || out_len == 0) {
        return;
    }
    out[0] = '\0';
    if (in == nullptr) {
        return;
    }
    strncpy(out, in, out_len - 1);
    out[out_len - 1] = '\0';
    char *p = strchr(out, '(');
    if (p) {
        *p = '\0';
    }
    /* Trim trailing spaces. */
    size_t n = strlen(out);
    while (n > 0 && isspace((unsigned char)out[n - 1])) {
        out[--n] = '\0';
    }
}

/** HTTP(S) GET; HTTPS uses setInsecure() (no cert verify). */
static bool http_get(const String &url, String &payload) {
    HTTPClient http;
    http.setTimeout(12000);
    http.setConnectTimeout(8000);
    http.setReuse(false);

    bool ok = false;
    WiFiClientSecure secure;
    WiFiClient plain;

    if (url.startsWith("https://")) {
        secure.setInsecure();
        const char *p = url.c_str() + 8;
        const char *slash = strchr(p, '/');
        if (slash == nullptr) {
            DBG_PRINTLN("[WX] bad https url");
            return false;
        }
        String host = String(p).substring(0, slash - p);
        String uri = String(slash);
        ok = http.begin(secure, host, 443, uri, true);
    } else if (url.startsWith("http://")) {
        const char *p = url.c_str() + 7;
        const char *slash = strchr(p, '/');
        if (slash == nullptr) {
            DBG_PRINTLN("[WX] bad http url");
            return false;
        }
        String host = String(p).substring(0, slash - p);
        String uri = String(slash);
        ok = http.begin(plain, host, 80, uri);
    } else {
        return false;
    }

    if (!ok) {
        DBG_PRINTLN("[WX] http.begin failed");
        return false;
    }

    http.setUserAgent("ESP32-WeatherClock/1.0");
    http.addHeader("Accept", "application/json");

    int code = http.GET();
    if (code != HTTP_CODE_OK) {
        DBG_PRINTF("[WX] HTTP %d  %s\n", code, url.c_str());
        String err = http.getString();
        if (err.length()) {
            DBG_PRINTLN(err);
        }
        http.end();
        return false;
    }

    payload = http.getString();
    http.end();
    return payload.length() > 0;
}

/** Extract numeric value after "key": ; skips non-numeric values like "wmo code". */
static bool json_find_number(const String &json, const char *key, double *out, int from = 0) {
    String pattern = String("\"") + key + "\":";
    int idx = from;
    while (true) {
        idx = json.indexOf(pattern, idx);
        if (idx < 0) {
            return false;
        }
        int val = idx + pattern.length();
        while (val < (int)json.length() && isspace((unsigned char)json.charAt(val))) {
            val++;
        }
        char c = (val < (int)json.length()) ? json.charAt(val) : '\0';
        if (c == '-' || c == '+' || isdigit((unsigned char)c)) {
            *out = atof(json.substring(val).c_str());
            return true;
        }
        /* e.g. "weather_code":"wmo code" in current_units — keep searching */
        idx = val + 1;
    }
}

/** Locate the "current":{...} object (not "current_units"). */
static int json_find_current_object(const String &json) {
    const char *keys[] = {"\"current\":{", "\"current\": {"};
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        int idx = json.indexOf(keys[i]);
        if (idx >= 0) {
            return idx;
        }
    }
    return -1;
}

/** Extract string value after "key":"..." in JSON text. */
static bool json_find_string(const String &json, const char *key, char *out, size_t out_len, int from = 0) {
    if (out == nullptr || out_len == 0) {
        return false;
    }
    String pattern = String("\"") + key + "\":\"";
    int idx = json.indexOf(pattern, from);
    if (idx < 0) {
        return false;
    }
    idx += pattern.length();
    int end = json.indexOf('"', idx);
    if (end < 0) {
        return false;
    }
    String val = json.substring(idx, end);
    strncpy(out, val.c_str(), out_len - 1);
    out[out_len - 1] = '\0';
    return true;
}

static void format_city_display(const char *city_only, const char *region,
                                char *out, size_t out_len) {
    (void)region;
    if (city_only) {
        strncpy(out, city_only, out_len - 1);
        out[out_len - 1] = '\0';
    } else {
        out[0] = '\0';
    }
}

static bool parse_lat_lon(const String &body, float *lat, float *lon,
                          const char *lat_key, const char *lon_key) {
    double dlat = 0, dlon = 0;
    if (!json_find_number(body, lat_key, &dlat) || !json_find_number(body, lon_key, &dlon)) {
        return false;
    }
    *lat = (float)dlat;
    *lon = (float)dlon;
    return true;
}

/** Multi-provider IP geolocation. */
static bool locate_by_ip(char *city, size_t city_len, float *lat, float *lon) {
    DBG_PRINTLN("[WX] IP locate...");

    struct Provider {
        const char *url;
        const char *lat_key;
        const char *lon_key;
        const char *region_key;
        bool need_success_field;
    };

    const Provider providers[] = {
        {"https://ipapi.co/json/", "latitude", "longitude", "region", false},
        {"https://api.ip.sb/geoip", "latitude", "longitude", "region", false},
        {"http://ip-api.com/json/?fields=status,message,city,regionName,lat,lon",
         "lat", "lon", "regionName", true},
    };

    for (size_t i = 0; i < sizeof(providers) / sizeof(providers[0]); i++) {
        const Provider &p = providers[i];
        DBG_PRINTF("[WX] try %s\n", p.url);

        String body;
        if (!http_get(p.url, body)) {
            continue;
        }

        if (p.need_success_field && body.indexOf("\"status\":\"success\"") < 0) {
            continue;
        }

        char city_only[40] = {0};
        char region[40] = {0};
        if (!json_find_string(body, "city", city_only, sizeof(city_only))) {
            continue;
        }
        json_find_string(body, p.region_key, region, sizeof(region));

        if (!parse_lat_lon(body, lat, lon, p.lat_key, p.lon_key)) {
            continue;
        }

        format_city_display(city_only, region, city, city_len);
        DBG_PRINTF("[WX] IP city: %s (%.4f, %.4f)\n", city, *lat, *lon);
        return true;
    }

    DBG_PRINTLN("[WX] All IP locate providers failed");
    return false;
}

static bool geocode_city(const char *city, float *lat, float *lon, char *name, size_t name_len) {
    String url = "https://geocoding-api.open-meteo.com/v1/search?name=";
    url += url_encode(city);
    url += "&count=1&language=en";

    DBG_PRINTF("[WX] Geocode: %s\n", city);

    String body;
    if (!http_get(url, body)) {
        return false;
    }

    if (body.indexOf("\"results\"") < 0) {
        DBG_PRINTLN("[WX] City not found");
        return false;
    }

    double dlat = 0, dlon = 0;
    if (!json_find_number(body, "latitude", &dlat) || !json_find_number(body, "longitude", &dlon)) {
        return false;
    }

    *lat = (float)dlat;
    *lon = (float)dlon;

    if (!json_find_string(body, "name", name, name_len)) {
        strncpy(name, city, name_len - 1);
        name[name_len - 1] = '\0';
    }

    DBG_PRINTF("[WX] Located: %s (%.4f, %.4f)\n", name, *lat, *lon);
    return true;
}

/** Fetch current temp / humidity / weather_code in one request. */
static bool fetch_current_weather(float lat, float lon,
                                  float *temp_c, int *humidity, int *weather_code) {
    char url[280];
    String body;

    DBG_PRINTLN("[WX] Fetch weather...");

    /* timezone=auto so "current" matches local wall clock; one round-trip. */
    snprintf(url, sizeof(url),
             "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
             "&current=temperature_2m,relative_humidity_2m,weather_code,"
             "precipitation,cloud_cover&timezone=auto",
             lat, lon);
    DBG_PRINTLN(url);
    if (!http_get(String(url), body)) {
        return false;
    }

    int current_idx = json_find_current_object(body);
    if (current_idx < 0) {
        DBG_PRINTLN("[WX] no current object");
        return false;
    }

    double temp = 0, hum = 0, code = 0, precip = 0, cloud = 0;
    if (!json_find_number(body, "temperature_2m", &temp, current_idx) ||
        !json_find_number(body, "relative_humidity_2m", &hum, current_idx) ||
        !json_find_number(body, "weather_code", &code, current_idx)) {
        DBG_PRINTLN("[WX] parse current fields failed");
        return false;
    }

    /* Optional fields — used to correct noisy drizzle codes under clear sky. */
    json_find_number(body, "precipitation", &precip, current_idx);
    json_find_number(body, "cloud_cover", &cloud, current_idx);

    int wmo = (int)(code + 0.5);

    if (wmo == 95 || wmo == 96 || wmo == 99) {
        int softened;
        if (precip < 0.15) {
            /* Near-zero rain: never keep Thunderstorm. */
            if (cloud <= 20.0) {
                softened = 0;      /* Clear */
            } else if (cloud <= 45.0) {
                softened = 1;      /* Mainly Clear */
            } else if (cloud <= 75.0) {
                softened = 2;      /* Partly Cloudy */
            } else {
                softened = 3;      /* Overcast */
            }
            DBG_PRINTF("[WX] thunder code=%d precip=%.2f cloud=%.0f%% -> %d (dry)\n",
                       wmo, precip, cloud, softened);
            wmo = softened;
        } else if (precip < 0.8 && cloud <= 80.0) {
            if (cloud <= 25.0) {
                softened = 1;
            } else if (cloud <= 55.0) {
                softened = 2;
            } else {
                softened = 3;
            }
            DBG_PRINTF("[WX] thunder code=%d precip=%.2f cloud=%.0f%% -> %d\n",
                       wmo, precip, cloud, softened);
            wmo = softened;
        }
    } else if ((wmo == 51 || wmo == 53 || wmo == 55 || wmo == 56 || wmo == 57) &&
               precip < 0.25 && cloud <= 50.0) {
        int softened = (cloud <= 20.0) ? 0 : ((cloud <= 40.0) ? 1 : 2);
        DBG_PRINTF("[WX] drizzle code=%d precip=%.2f cloud=%.0f%% -> %d\n",
                   wmo, precip, cloud, softened);
        wmo = softened;
    } else if ((wmo >= 80 && wmo <= 82) && precip < 0.25 && cloud <= 50.0) {
        int softened = (cloud <= 20.0) ? 0 : ((cloud <= 40.0) ? 1 : 2);
        DBG_PRINTF("[WX] shower code=%d precip=%.2f cloud=%.0f%% -> %d\n",
                   wmo, precip, cloud, softened);
        wmo = softened;
    } else if ((wmo == 61 || wmo == 63 || wmo == 65) &&
               precip < 0.15 && cloud <= 40.0) {
        int softened = (cloud <= 20.0) ? 0 : 1;
        DBG_PRINTF("[WX] rain code=%d precip=%.2f cloud=%.0f%% -> %d\n",
                   wmo, precip, cloud, softened);
        wmo = softened;
    }

    *temp_c = (float)temp;
    *humidity = (int)(hum + 0.5);
    *weather_code = wmo;
    DBG_PRINTF("[WX] parsed code=%d temp=%.1f hum=%d precip=%.2f cloud=%.0f%%\n",
               wmo, temp, *humidity, precip, cloud);
    return true;
}

const char *weather_condition_text(int weather_code) {
    switch (weather_code) {
        case 0:  return "Clear";
        case 1:  return "Mainly Clear";
        case 2:  return "Partly Cloudy";
        case 3:  return "Overcast";
        case 45:
        case 48: return "Fog";
        case 51:
        case 53:
        case 55: return "Drizzle";
        case 56:
        case 57: return "Freezing Drizzle";
        case 61: return "Light Rain";
        case 63: return "Rain";
        case 65: return "Heavy Rain";
        case 66:
        case 67: return "Freezing Rain";
        case 71: return "Light Snow";
        case 73: return "Snow";
        case 75: return "Heavy Snow";
        case 77: return "Snow Grains";
        case 80: return "Light Showers";
        case 81: return "Showers";
        case 82: return "Heavy Showers";
        case 85:
        case 86: return "Snow Showers";
        case 95: return "Thunderstorm";
        case 96:
        case 99: return "Thunderstorm Hail";
        default: return "Unknown";
    }
}

static bool weather_fill(float lat, float lon, bool from_ip) {
    float temp = 0;
    int humidity = 0;
    int code = 0;
    if (!fetch_current_weather(lat, lon, &temp, &humidity, &code)) {
        return false;
    }

    g_weather.latitude = lat;
    g_weather.longitude = lon;
    g_weather.temperature_c = temp;
    g_weather.humidity_pct = humidity;
    g_weather.weather_code = code;
    strncpy(g_weather.condition, weather_condition_text(code), sizeof(g_weather.condition) - 1);
    g_weather.condition[sizeof(g_weather.condition) - 1] = '\0';
    g_weather.from_ip = from_ip;
    g_weather.valid = true;
    weather_print();
    return true;
}

/**
 * Auto locate then fetch weather.
 * Order: IP locate -> EEPROM coords -> city geocode.
 */
bool weather_update_auto(void) {
    weather_clear();

    if (WiFi.status() != WL_CONNECTED) {
        DBG_PRINTLN("[WX] WiFi not connected");
        return false;
    }

    char city[CONFIG_CITY_LEN];
    float lat = 0, lon = 0;

    if (locate_by_ip(city, sizeof(city), &lat, &lon)) {
        strncpy(g_weather.query_city, city, sizeof(g_weather.query_city) - 1);
        strncpy(g_weather.resolved_name, city, sizeof(g_weather.resolved_name) - 1);
        config_store_save_location(city, lat, lon);
        return weather_fill(lat, lon, true);
    }

    const AppConfig &cfg = config_store_get();

    if (config_store_has_coords()) {
        DBG_PRINTF("[WX] Fallback to cached coords %.4f,%.4f (%s)\n",
                      cfg.latitude, cfg.longitude,
                      cfg.city[0] ? cfg.city : "?");
        strncpy(g_weather.query_city, cfg.city, sizeof(g_weather.query_city) - 1);
        strncpy(g_weather.resolved_name, cfg.city, sizeof(g_weather.resolved_name) - 1);
        return weather_fill(cfg.latitude, cfg.longitude, false);
    }

    if (cfg.city[0] != '\0') {
        char base[CONFIG_CITY_LEN];
        city_basename(cfg.city, base, sizeof(base));
        DBG_PRINTF("[WX] Fallback geocode: %s\n", base);
        if (base[0] != '\0' && weather_update(base)) {
            config_store_save_location(
                g_weather.resolved_name[0] ? g_weather.resolved_name : base,
                g_weather.latitude,
                g_weather.longitude);
            return true;
        }
    }

    DBG_PRINTLN("[WX] Auto locate failed");
    return false;
}

bool weather_update(const char *city) {
    weather_clear();

    if (WiFi.status() != WL_CONNECTED) {
        DBG_PRINTLN("[WX] WiFi not connected");
        return false;
    }
    if (city == nullptr || city[0] == '\0') {
        DBG_PRINTLN("[WX] Empty city");
        return false;
    }

    strncpy(g_weather.query_city, city, sizeof(g_weather.query_city) - 1);

    float lat = 0, lon = 0;
    if (!geocode_city(city, &lat, &lon, g_weather.resolved_name, sizeof(g_weather.resolved_name))) {
        return false;
    }

    return weather_fill(lat, lon, false);
}

const WeatherInfo &weather_get(void) {
    return g_weather;
}

void weather_print(void) {
    if (!g_weather.valid) {
        DBG_PRINTLN("[WX] no data");
        return;
    }
    DBG_PRINTF("[WX] %s%s  %s  %.1fC  %d%%RH  (code=%d)\n",
                  g_weather.resolved_name[0] ? g_weather.resolved_name : g_weather.query_city,
                  g_weather.from_ip ? " (IP)" : "",
                  g_weather.condition,
                  g_weather.temperature_c,
                  g_weather.humidity_pct,
                  g_weather.weather_code);
}
