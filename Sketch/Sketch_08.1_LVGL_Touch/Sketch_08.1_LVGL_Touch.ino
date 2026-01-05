/*
* @ File:   Sketch_08.1_LVGL.ino
* @ Author: [Vegetable-SYC]
* @ Date:   [2025-12-19]
*/

#include <lvgl.h>
#include <TFT_eSPI.h>

/* --- Configuration Parameters --- */
const int TOUCH_PIN = 32;          // Capacitive touch GPIO
const int TFT_VCC = 21;            // Power control pin
const int TOUCH_THRESHOLD = 80;    // Sensitivity threshold
const int PRESS_Time = 3000;       // Long press threshold (3000ms)
int touch_base_val = 0;            // Initial baseline value

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 240

TFT_eSPI tft = TFT_eSPI(); 
static uint8_t buf[SCREEN_WIDTH * SCREEN_HEIGHT / 10 * 2]; // LVGL draw buffer

lv_obj_t * label_short;
lv_obj_t * label_long;
int short_press_num = 0;
int long_press_num = 0;

/* --- 1. Display Flush Callback --- */
void my_disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)px_map, w * h, true);
    tft.endWrite();
    lv_display_flush_ready(disp); // Inform LVGL flushing is complete
}

/* --- 2. Input Device Read Callback --- */
void my_touchpad_read(lv_indev_t * indev, lv_indev_data_t * data) {
    int val = touchRead(TOUCH_PIN);
    
    // Determine state based on baseline comparison
    if (touch_base_val - val > TOUCH_THRESHOLD) {
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
    data->btn_id = 0; // Use the first button mapping
}

/* --- 3. UI Event Callback --- */
void btn_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED) {
        // Triggered upon releasing after a short press
        short_press_num++;
        lv_label_set_text_fmt(label_short, "Short Press: %d", short_press_num);
        Serial.println("Short Press Detected");
    }
    else if(code == LV_EVENT_LONG_PRESSED) {
        // Triggered upon releasing after a long press
        long_press_num++;
        lv_label_set_text_fmt(label_long, "Long Press: %d", long_press_num);
        Serial.println("Long Press Detected (3s)");
    }
}

void setup() {
    Serial.begin(115200);

    // Initialize capacitive touch hardware
    touchSetCycles(0xf000, 0x1000);
    touch_base_val = touchRead(TOUCH_PIN); 

    tft.begin();
    lv_init();

    // Register LVGL display driver
    lv_display_t * disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Register Input Device (Button Type for single touch point)
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_BUTTON);
    lv_indev_set_read_cb(indev, my_touchpad_read);

    // Set LVGL long press detection threshold
    lv_indev_set_long_press_time(indev, PRESS_Time);

    // Map the hardware touch sensor to a virtual screen coordinate
    static const lv_point_t btn_points[] = { {120, 150} }; // Coordinates within button area
    lv_indev_set_button_points(indev, btn_points);

    // Create User Interface - Interactive Button
    lv_obj_t * btn = lv_button_create(lv_screen_active());
    lv_obj_set_size(btn, 180, 80);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 30);
    
    // Listen for all interaction events
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t * btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Touch Pin 32");
    lv_obj_center(btn_label);

    // Create Feedback Labels
    label_short = lv_label_create(lv_screen_active());
    lv_label_set_text(label_short, "Short Press: 0");
    lv_obj_align(label_short, LV_ALIGN_TOP_MID, 0, 40);

    label_long = lv_label_create(lv_screen_active());
    lv_label_set_text(label_long, "Long Press: 0");
    lv_obj_align(label_long, LV_ALIGN_TOP_MID, 0, 70);

    // Power up display
    pinMode(TFT_VCC, OUTPUT);
    digitalWrite(TFT_VCC, LOW); 
}

void loop() {
    // Process UI tasks
    lv_timer_handler();
    
    // Handle LVGL internal tick timing
    static uint32_t last_tick = 0;
    uint32_t current_ms = millis();
    lv_tick_inc(current_ms - last_tick);
    last_tick = current_ms;

    delay(5);
}