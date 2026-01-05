/*
* @ File:   Sketch_10.1_LVGL_Timer.ino
* @ Author: [Vegetable-SYC]
* @ Date:   [2025-12-24]
*/

#include <lvgl.h>
#include <TFT_eSPI.h>

/* Hardware mapping & Constants */
const int TOUCH_PIN = 32;          // Capacitive touch GPIO
const int TFT_VCC = 21;            // Display power control
const int TOUCH_THRESHOLD = 80;    // Sensitivity calibration
const int PRESS_TIME = 3000;       // Reset threshold (ms)
int touch_base_val = 0;            // Auto-calibrated baseline

/* Display specifications */
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 240

TFT_eSPI tft = TFT_eSPI(); 

/* Memory-aligned draw buffer for optimized DMA/rendering */
static uint32_t buf_aligned[ (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * 2) / 4 + 1 ];
uint8_t * draw_buf_ptr = (uint8_t *)buf_aligned;

/* Global state machine & variables */
bool is_running = false;
uint32_t elapsed_time = 0;   
uint32_t last_millis = 0;    

/* UI Object handles */
lv_obj_t * label_time;       
lv_obj_t * label_status;     
lv_obj_t * arc_deco;       

/**
 * Flush callback: Bridges LVGL software rendering to hardware SPI
 */
void my_disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)px_map, w * h, true);
    tft.endWrite();
    lv_display_flush_ready(disp);
}

/**
 * Input device callback: Logic for capacitive touch state acquisition
 */
void my_touchpad_read(lv_indev_t * indev, lv_indev_data_t * data) {
    int val = touchRead(TOUCH_PIN);
    if (touch_base_val - val > TOUCH_THRESHOLD) {
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
    data->btn_id = 0; 
}

/**
 * Timer callback: Manages chronological calculations and visual effects
 */
void timer_refresh_cb(lv_timer_t * timer) {
    if (is_running) {
        uint32_t now = millis();
        elapsed_time += (now - last_millis);
        last_millis = now;

        /* Calculate clock components */
        uint32_t sec = (elapsed_time / 1000) % 60;
        uint32_t min = (elapsed_time / 60000) % 60;
        uint32_t ms = (elapsed_time % 1000) / 100;
        lv_label_set_text_fmt(label_time, "%02d:%02d.%d", min, sec, ms);

        /* Procedural arc animation */
        static uint16_t angle = 0;
        angle = (angle + 4) % 360;
        lv_arc_set_rotation(arc_deco, angle);
    }
}

/**
 * Interaction event manager: Handles toggle and reset logic
 */
void btn_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_SHORT_CLICKED) {
        is_running = !is_running;
        if(is_running) {
            last_millis = millis();
            lv_label_set_text(label_status, "RUNNING");
            lv_obj_set_style_text_color(label_status, lv_palette_main(LV_PALETTE_GREEN), 0);
            lv_obj_set_style_arc_color(arc_deco, lv_palette_main(LV_PALETTE_GREEN), LV_PART_INDICATOR);
        } else {
            lv_label_set_text(label_status, "PAUSED");
            lv_obj_set_style_text_color(label_status, lv_palette_main(LV_PALETTE_ORANGE), 0);
            lv_obj_set_style_arc_color(arc_deco, lv_palette_main(LV_PALETTE_ORANGE), LV_PART_INDICATOR);
        }
    }
    else if(code == LV_EVENT_LONG_PRESSED) {
        /* System Reset Sequence */
        is_running = false;
        elapsed_time = 0;
        lv_label_set_text(label_time, "00:00.0");
        lv_label_set_text(label_status, "READY");
        lv_obj_set_style_text_color(label_status, lv_palette_main(LV_PALETTE_CYAN), 0);
        lv_obj_set_style_arc_color(arc_deco, lv_palette_main(LV_PALETTE_CYAN), LV_PART_INDICATOR);
        Serial.println("System Reset Successful");
    }
}

void setup() {
    Serial.begin(115200);

    /* Hardware Layer Init */
    tft.begin();
    tft.fillScreen(TFT_BLACK); 
    touchSetCycles(0xf000, 0x1000);
    touch_base_val = touchRead(TOUCH_PIN); 

    /* Framework Layer Init */
    lv_init();

    /* Display Registration */
    lv_display_t * disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, draw_buf_ptr, NULL, (SCREEN_WIDTH * SCREEN_HEIGHT / 10) * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Input Device Configuration */
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_BUTTON);
    lv_indev_set_read_cb(indev, my_touchpad_read);
    lv_indev_set_long_press_time(indev, PRESS_TIME);

    /* Map hardware sensor to logical button area */
    static const lv_point_t btn_points[] = { {120, 120} }; 
    lv_indev_set_button_points(indev, btn_points);

    /* UI Theme & Orchestration */
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);

    /* Stylized Decorative Arc */
    arc_deco = lv_arc_create(scr);
    lv_obj_set_size(arc_deco, 220, 220);
    lv_arc_set_range(arc_deco, 0, 100);
    lv_arc_set_value(arc_deco, 15);
    lv_arc_set_bg_angles(arc_deco, 0, 360);
    lv_obj_set_style_arc_width(arc_deco, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_deco, 6, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_deco, lv_color_hex(0x1A1A1A), LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_deco, lv_palette_main(LV_PALETTE_CYAN), LV_PART_INDICATOR);
    lv_obj_remove_style(arc_deco, NULL, LV_PART_KNOB); 
    lv_obj_center(arc_deco);

    /* Button Interaction */
    lv_obj_t * btn_full = lv_button_create(scr);
    lv_obj_set_size(btn_full, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_style_bg_opa(btn_full, 0, 0); 
    lv_obj_set_style_border_opa(btn_full, 0, 0);
    lv_obj_set_style_shadow_opa(btn_full, 0, 0);
    lv_obj_add_event_cb(btn_full, btn_event_cb, LV_EVENT_ALL, NULL);

    /* Chrono Readout Setup */
    label_time = lv_label_create(scr);
    lv_label_set_text(label_time, "00:00.0");
    lv_obj_set_style_text_font(label_time, &lv_font_montserrat_40, 0); 
    lv_obj_set_style_text_color(label_time, lv_color_white(), 0);
    lv_obj_align(label_time, LV_ALIGN_TOP_LEFT, 50, 95); 
    lv_obj_set_style_text_align(label_time, LV_TEXT_ALIGN_LEFT, 0);

    /* Secondary Information Readout */
    label_status = lv_label_create(scr);
    lv_label_set_text(label_status, "READY");
    lv_obj_set_style_text_font(label_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label_status, lv_palette_main(LV_PALETTE_CYAN), 0);
    lv_obj_align(label_status, LV_ALIGN_CENTER, 0, 45);

    /* Design separation line */
    lv_obj_t * line = lv_obj_create(scr);
    lv_obj_set_size(line, 80, 2);
    lv_obj_set_style_bg_color(line, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_opa(line, 0, 0);
    lv_obj_align(line, LV_ALIGN_CENTER, 0, 20);

    /* Primary Application Task Initiation */
    lv_timer_create(timer_refresh_cb, 50, NULL);

    /* TFT Power Engagement */
    pinMode(TFT_VCC, OUTPUT);
    digitalWrite(TFT_VCC, LOW); 
}

void loop() {
    /* Handle pending framework tasks and signal management */
    lv_timer_handler();
    
    /* System synchronization - synchronize library clock to hardware millis */
    static uint32_t last_tick = 0;
    uint32_t current_ms = millis();
    lv_tick_inc(current_ms - last_tick);
    last_tick = current_ms;

    delay(5);
}