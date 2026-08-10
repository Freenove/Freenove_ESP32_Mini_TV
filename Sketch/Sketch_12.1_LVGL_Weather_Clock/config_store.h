#ifndef CONFIG_STORE_H
#define CONFIG_STORE_H

#include <Arduino.h>

#define CONFIG_MAGIC       0xA5C0FF05UL
#define CONFIG_SSID_LEN    33
#define CONFIG_PASS_LEN    65
#define CONFIG_TZ_LEN      40
#define CONFIG_CITY_LEN    48
#define CONFIG_DEFAULT_TZ  "UTC-8"

#define THEME_DARK  0
#define THEME_LIGHT 1

/* Main UI face. */
#define UI_FACE_WEATHER  0
#define UI_FACE_ANALOG   1
#define UI_FACE_XL       2 

/* Clock hour format. */
#define HOUR_FMT_24  0
#define HOUR_FMT_12  1

/* Date field order. */
#define DATE_ORDER_YMD  0  /* year-month-day */
#define DATE_ORDER_MDY  1  /* month-day-year */
#define DATE_ORDER_DMY  2  /* day-month-year */

/* Month presentation. */
#define MONTH_NUM  0  /* 01..12 */
#define MONTH_ENG  1  /* Jan..Dec */

struct AppConfig {
    uint32_t magic;
    char ssid[CONFIG_SSID_LEN];
    char password[CONFIG_PASS_LEN];
    char timezone[CONFIG_TZ_LEN];
    char city[CONFIG_CITY_LEN];
    float latitude;
    float longitude;
    uint8_t has_coords;
    uint8_t theme;
    uint8_t hour12;
    uint8_t date_order;
    uint8_t month_en;
    uint8_t ui_face;
};

void config_store_begin(void);
bool config_store_is_valid(void);
const AppConfig &config_store_get(void);

bool config_store_save(const char *ssid, const char *password, const char *timezone, const char *city);
bool config_store_save_timezone(const char *timezone);
bool config_store_save_location(const char *city, float latitude, float longitude);
bool config_store_has_coords(void);
bool config_store_save_theme(uint8_t theme);
bool config_store_save_display_format(uint8_t hour12, uint8_t date_order, uint8_t month_en);
bool config_store_save_ui_face(uint8_t ui_face);
void config_store_clear(void);

#endif
