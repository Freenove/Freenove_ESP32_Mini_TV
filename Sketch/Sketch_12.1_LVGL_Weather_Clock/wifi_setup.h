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
#define WIFI_PORTAL_TIMEOUT_S 300
#endif

#ifndef WIFI_CONNECT_TIMEOUT_S
#define WIFI_CONNECT_TIMEOUT_S 15
#endif

/** Progress callback for Serial / UI */
typedef void (*WifiStatusCallback)(const char *text);

/** Called often while connecting / while SoftAP portal is open (LVGL + touch). */
typedef void (*WifiIdleCallback)(void);

bool wifi_setup_begin(WifiStatusCallback on_status = nullptr,
                      WifiIdleCallback on_idle = nullptr);

/** Clear EEPROM and WiFiManager saved credentials. */
void wifi_setup_reset(void);

#endif
