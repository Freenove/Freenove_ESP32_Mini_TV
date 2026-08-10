#include "ntp_time.h"
#include "debug_log.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

static const char *TZ_DEFAULT_POSIX = "UTC-8";

static const char *skip_ws(const char *s) {
    while (s && *s && isspace((unsigned char)*s)) {
        ++s;
    }
    return s;
}

static bool looks_like_offset_number(const char *s) {
    s = skip_ws(s);
    if (s == nullptr || *s == '\0') {
        return false;
    }
    if (*s == '+' || *s == '-') {
        ++s;
    }
    if (!isdigit((unsigned char)*s)) {
        return false;
    }
    while (*s) {
        if (isalpha((unsigned char)*s)) {
            return false;
        }
        if (!isdigit((unsigned char)*s) && !isspace((unsigned char)*s) &&
            *s != '.' && *s != ':') {
            return false;
        }
        ++s;
    }
    return true;
}

static void format_posix_from_utc_offset(int utc_hours, char *out, size_t outlen) {
    if (out == nullptr || outlen == 0) {
        return;
    }
    if (utc_hours < -12) {
        utc_hours = -12;
    }
    if (utc_hours > 14) {
        utc_hours = 14;
    }
    const int posix = -utc_hours;
    if (posix > 0) {
        snprintf(out, outlen, "UTC+%d", posix);
    } else {
        snprintf(out, outlen, "UTC%d", posix);
    }
}

int ntp_timezone_utc_offset(const char *tz) {
    const char *s = skip_ws(tz);
    if (s == nullptr || *s == '\0') {
        return 8;
    }

    if (looks_like_offset_number(s)) {
        int v = atoi(s);
        if (v < -12) v = -12;
        if (v > 14) v = 14;
        return v;
    }

    while (*s && isalpha((unsigned char)*s)) {
        ++s;
    }
    if (*s == '\0') {
        return 8;
    }
    int utc = -atoi(s);
    if (utc < -12) utc = -12;
    if (utc > 14) utc = 14;
    return utc;
}

bool ntp_timezone_normalize(const char *input, char *out, size_t outlen) {
    if (out == nullptr || outlen < 8) {
        return false;
    }

    const char *s = skip_ws(input);
    if (s == nullptr || *s == '\0') {
        strncpy(out, TZ_DEFAULT_POSIX, outlen - 1);
        out[outlen - 1] = '\0';
        return false;
    }

    if (looks_like_offset_number(s)) {
        format_posix_from_utc_offset(atoi(s), out, outlen);
        return true;
    }

    strncpy(out, s, outlen - 1);
    out[outlen - 1] = '\0';
    size_t n = strlen(out);
    while (n > 0 && isspace((unsigned char)out[n - 1])) {
        out[--n] = '\0';
    }
    return true;
}

bool ntp_time_begin(const char *timezone) {
    char tz_buf[24];
    ntp_timezone_normalize(timezone, tz_buf, sizeof(tz_buf));

    configTzTime(tz_buf, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);

    DBG_PRINTF("[NTP] Syncing, TZ=%s (UTC%+d) ...\n",
                  tz_buf, ntp_timezone_utc_offset(tz_buf));

    struct tm timeinfo;
    const uint32_t deadline = millis() + 15000UL;

    while (millis() < deadline) {
        if (getLocalTime(&timeinfo, 1000)) {
            if (timeinfo.tm_year + 1900 >= 2024) {
                DBG_PRINTLN("[NTP] Synced OK");
                ntp_time_print();
                return true;
            }
        }
        DBG_PRINT('.');
    }

    DBG_PRINTLN();
    DBG_PRINTLN("[NTP] Sync timeout");
    return false;
}

bool ntp_time_is_synced(void) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 50)) {
        return false;
    }
    return (timeinfo.tm_year + 1900) >= 2024;
}

bool ntp_time_get(struct tm *out) {
    if (out == nullptr) {
        return false;
    }
    return getLocalTime(out, 50);
}

bool ntp_time_format(char *buf, size_t buflen) {
    if (buf == nullptr || buflen < 20) {
        return false;
    }
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 50)) {
        return false;
    }
    strftime(buf, buflen, "%Y-%m-%d %H:%M:%S", &timeinfo);
    return true;
}

void ntp_time_print(void) {
    char buf[32];
    if (ntp_time_format(buf, sizeof(buf))) {
        DBG_PRINTF("[TIME] %s\n", buf);
    } else {
        DBG_PRINTLN("[TIME] not synced");
    }
}
