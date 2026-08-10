#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include <Arduino.h>

#ifndef WIFI_AP_NAME
#define WIFI_AP_NAME "WeatherClock-AP"
#endif

#ifndef WIFI_AP_PASSWORD
#define WIFI_AP_PASSWORD ""
#endif

#ifndef WIFI_PORTAL_TIMEOUT_S
#define WIFI_PORTAL_TIMEOUT_S 180
#endif

#ifndef WIFI_CONNECT_TIMEOUT_S
#define WIFI_CONNECT_TIMEOUT_S 15
#endif

/** Progress callback for Serial / UI */
typedef void (*WifiStatusCallback)(const char *text);

bool wifi_setup_begin(WifiStatusCallback on_status = nullptr);

/** Clear EEPROM and WiFiManager saved credentials. */
void wifi_setup_reset(void);

#endif
