#include "touch_button.h"
#include "debug_log.h"

static int s_base = 0;
static bool s_pressed = false;
static bool s_theme_fired = false;
static bool s_hint_fired = false;
static bool s_reconfig_fired = false;
static uint32_t s_press_start_ms = 0;

void touch_button_begin(void) {
    touchSetCycles(0xf000, 0x1000);
    delay(20);
    s_base = (int)touchRead(TOUCH_PIN);
    s_pressed = false;
    s_theme_fired = false;
    s_hint_fired = false;
    s_reconfig_fired = false;
    s_press_start_ms = 0;
    DBG_PRINTF("[TOUCH] pin=%d base=%d thr=%d short=%ums theme=%ums "
               "hint=%ums reconfig=%ums\n",
               TOUCH_PIN, s_base, TOUCH_THRESHOLD,
               (unsigned)TOUCH_SHORT_MIN_MS, (unsigned)TOUCH_THEME_PRESS_MS,
               (unsigned)TOUCH_RECONFIG_HINT_MS, (unsigned)TOUCH_RECONFIG_PRESS_MS);
}

bool touch_button_is_pressed(void) {
    int val = (int)touchRead(TOUCH_PIN);
    return (s_base - val) > TOUCH_THRESHOLD;
}

uint32_t touch_button_held_ms(void) {
    if (!s_pressed) {
        return 0;
    }
    return millis() - s_press_start_ms;
}

TouchEvent touch_button_poll(void) {
    bool down = touch_button_is_pressed();
    uint32_t now = millis();

    if (down) {
        if (!s_pressed) {
            s_pressed = true;
            s_theme_fired = false;
            s_hint_fired = false;
            s_reconfig_fired = false;
            s_press_start_ms = now;
            return TOUCH_EVENT_NONE;
        }

        uint32_t held = now - s_press_start_ms;

        if (!s_theme_fired && held >= TOUCH_THEME_PRESS_MS) {
            s_theme_fired = true;
            DBG_PRINTLN("[TOUCH] Theme long-press (3s)");
            return TOUCH_EVENT_THEME;
        }

        if (!s_hint_fired && held >= TOUCH_RECONFIG_HINT_MS) {
            s_hint_fired = true;
            DBG_PRINTLN("[TOUCH] Reconfig hint (6s)");
            return TOUCH_EVENT_RECONFIG_HINT;
        }

        if (!s_reconfig_fired && held >= TOUCH_RECONFIG_PRESS_MS) {
            s_reconfig_fired = true;
            DBG_PRINTLN("[TOUCH] Reconfig long-press (10s)");
            return TOUCH_EVENT_RECONFIG;
        }

        return TOUCH_EVENT_NONE;
    }

    if (s_pressed) {
        uint32_t held = now - s_press_start_ms;
        bool want_face = !s_theme_fired && (held >= TOUCH_SHORT_MIN_MS);
        s_pressed = false;
        s_theme_fired = false;
        s_hint_fired = false;
        s_reconfig_fired = false;
        s_press_start_ms = 0;
        if (want_face) {
            DBG_PRINTLN("[TOUCH] Face short-press");
            return TOUCH_EVENT_FACE;
        }
    }
    return TOUCH_EVENT_NONE;
}
