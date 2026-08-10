#ifndef WEATHER_H
#define WEATHER_H

#include <Arduino.h>

struct WeatherInfo {
    char query_city[48];
    char resolved_name[48];
    float latitude;
    float longitude;
    float temperature_c;
    int humidity_pct;
    int weather_code;
    char condition[24];
    bool valid;
    bool from_ip;
};

bool weather_update_auto(void);
bool weather_update(const char *city);
const WeatherInfo &weather_get(void);
void weather_print(void);

/** Map WMO weather_code to a short English label. */
const char *weather_condition_text(int weather_code);

#endif /* WEATHER_H */
