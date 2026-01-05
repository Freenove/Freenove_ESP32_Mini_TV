/*
* @ File:   Sketch_07.1_LVGL.ino
* @ Author: [Vegetable-SYC]
* @ Date:   [2025-12-12]
*/

#include <lvgl.h>
#include <TFT_eSPI.h>

/* Display resolution configuration */
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 240

/* Hardware and driver initialization */
const int TFT_VCC = 21;
TFT_eSPI tft = TFT_eSPI(); 

/* LVGL display handles and draw buffers */
static lv_display_t * disp;
static uint8_t buf[SCREEN_WIDTH * SCREEN_HEIGHT / 10 * 2];

/**
 * Flush callback: Transfers pixel data from LVGL to the TFT controller
 */
void my_disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)px_map, w * h, true);
    tft.endWrite();

    lv_display_flush_ready(disp); // Indicate flushing finished
}

void setup() {
    Serial.begin(115200);

    /* Initialize physical display */
    tft.begin();
    
    /* Initialize LVGL core library */
    lv_init();

    /* Setup display abstraction layer */
    disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* UI initialization and version logging */
    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.println(LVGL_Arduino);
    Serial.println("System Ready: LVGL initialized");

    /* Create and center-align a label widget */
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, LVGL_Arduino.c_str());
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    /* Enable display power */
    pinMode(TFT_VCC, OUTPUT);
    digitalWrite(TFT_VCC, LOW);
}

void loop() {
    /* Update library tick and process pending UI tasks */
    lv_tick_inc(5); 
    lv_timer_handler(); 
    delay(5);
}