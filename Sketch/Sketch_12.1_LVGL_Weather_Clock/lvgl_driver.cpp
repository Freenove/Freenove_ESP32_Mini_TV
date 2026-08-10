#include "lvgl_driver.h"

#include <TFT_eSPI.h>

#ifndef TFT_VCC_PIN
#define TFT_VCC_PIN 21
#endif

#ifndef LVGL_TICK_MS
#define LVGL_TICK_MS 5
#endif

#ifndef LVGL_DRAW_BUF_LINES
#define LVGL_DRAW_BUF_LINES 10
#endif

static TFT_eSPI tft;
static lv_display_t *disp = nullptr;
static uint8_t draw_buf[SCREEN_WIDTH * LVGL_DRAW_BUF_LINES * sizeof(uint16_t)];

static void lvgl_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)px_map, w * h, true);
    tft.endWrite();

    lv_display_flush_ready(display);
}

void lvgl_driver_init(void) {
    pinMode(TFT_VCC_PIN, OUTPUT);
    digitalWrite(TFT_VCC_PIN, LOW);

    tft.begin();
    lv_init();

    disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_flush_cb(disp, lvgl_flush_cb);
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
}

void lvgl_driver_handler(void) {
    lv_tick_inc(LVGL_TICK_MS);
    lv_timer_handler();
}
