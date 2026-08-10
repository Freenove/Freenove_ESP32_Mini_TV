#ifndef TOUCH_BUTTON_H
#define TOUCH_BUTTON_H

#include <Arduino.h>

#ifndef TOUCH_PIN
#define TOUCH_PIN 32
#endif

#ifndef TOUCH_THRESHOLD
#define TOUCH_THRESHOLD 80
#endif

/** Ignore taps shorter than this (ms). */
#ifndef TOUCH_SHORT_MIN_MS
#define TOUCH_SHORT_MIN_MS 40
#endif

/** Hold continuously: switch theme */
#ifndef TOUCH_THEME_PRESS_MS
#define TOUCH_THEME_PRESS_MS 1000
#endif

enum TouchEvent : uint8_t {
    TOUCH_EVENT_NONE = 0,
    TOUCH_EVENT_FACE,
    TOUCH_EVENT_THEME,
};

void touch_button_begin(void);

/** Poll touch **/
TouchEvent touch_button_poll(void);

bool touch_button_is_pressed(void);

#endif
