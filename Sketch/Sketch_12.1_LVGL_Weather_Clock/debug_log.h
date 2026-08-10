#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#ifndef SERIAL_DEBUG
#define SERIAL_DEBUG 0
#endif

#if SERIAL_DEBUG
#include <Arduino.h>
#define DBG_BEGIN(...)    Serial.begin(__VA_ARGS__)
#define DBG_PRINT(...)    Serial.print(__VA_ARGS__)
#define DBG_PRINTLN(...)  Serial.println(__VA_ARGS__)
#define DBG_PRINTF(...)   Serial.printf(__VA_ARGS__)
#else
#define DBG_BEGIN(...)    ((void)0)
#define DBG_PRINT(...)    ((void)0)
#define DBG_PRINTLN(...)  ((void)0)
#define DBG_PRINTF(...)   ((void)0)
#endif

#endif
