#ifndef NTP_TIME_H
#define NTP_TIME_H

#include <Arduino.h>
#include <time.h>

#ifndef NTP_SERVER_1
#define NTP_SERVER_1 "ntp.aliyun.com"
#endif
#ifndef NTP_SERVER_2
#define NTP_SERVER_2 "pool.ntp.org"
#endif
#ifndef NTP_SERVER_3
#define NTP_SERVER_3 "time.nist.gov"
#endif

bool ntp_timezone_normalize(const char *input, char *out, size_t outlen);
int ntp_timezone_utc_offset(const char *tz);
bool ntp_time_begin(const char *timezone);

bool ntp_time_is_synced(void);
bool ntp_time_get(struct tm *out);
bool ntp_time_format(char *buf, size_t buflen);
void ntp_time_print(void);

#endif
