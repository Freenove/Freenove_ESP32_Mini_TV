/*
* @ File:   Sketch_11.1_LVGL_Game.ino
* @ Author: [Vegetable-SYC]
* @ Date:   [2025-12-24]
*/

#include <lvgl.h>
#include <TFT_eSPI.h>
#include "image.h"
#include <EEPROM.h>

/* --- Hardware Configuration --- */
const int TOUCH_PIN = 32;
const int TFT_VCC = 21;
const int TOUCH_THRESHOLD = 80;
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 240
#define EEPROM_SIZE 4

/* --- Graphics Instances --- */
TFT_eSPI tft = TFT_eSPI(); 
static uint32_t buf_aligned[ (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * 2) / 4 + 4 ];

/* --- LVGL Image Descriptor --- */
const lv_image_dsc_t img_bg = {
  .header = {
    .magic = LV_IMAGE_HEADER_MAGIC,  
    .cf = LV_COLOR_FORMAT_RGB565,    
    .flags = 0,
    .w = 240,                        
    .h = 240,                        
    .stride = 240 * 2,               
    .reserved_2 = 0,
  },
  .data_size = sizeof(img_bg_data),     
  .data = img_bg_data,                  
  .reserved = NULL,
};

/* --- Game Physics & Constants --- */
const int GRAVITY = 1;
const int JUMP_FORCE = -9;
const int PIPE_SPEED = 4;
const int PIPE_GAP = 100;   
const int PIPE_WIDTH = 40;

/* --- Game State Variables --- */
int bird_y = 120;
int bird_vel = 0;
int pipe_x = 240;
int pipe_gap_y = 100;
int score = 0;
bool game_running = false;
int touch_base_val = 0;
int best_score = 0; 

/* --- UI Object References --- */
lv_obj_t * bird = NULL;
lv_obj_t * pipe_top = NULL;
lv_obj_t * pipe_bottom = NULL;
lv_obj_t * label_score = NULL;
lv_obj_t * label_msg = NULL;

/**
 * @brief Load best score from EEPROM
 */
void load_best_score() {
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(0, best_score);
    if (best_score < 0 || best_score > 9999) { 
        best_score = 0; 
    }
}

/**
 * @brief Handle game termination logic
 */
void game_over_action() {
    game_running = false;

    bool is_new_record = false;
    if (score > best_score) {
        best_score = score;
        EEPROM.put(0, best_score);
        EEPROM.commit();
        is_new_record = true;
    }

    lv_label_set_text_fmt(label_msg, 
        "%s\nSCORE: %d\nBEST: %d", 
        is_new_record ? "#008000 NEW RECORD!#" : "#FF0000 GAME OVER#", 
        score, 
        best_score);

    lv_label_set_recolor(label_msg, true); 
    lv_obj_remove_flag(label_msg, LV_OBJ_FLAG_HIDDEN);
}

/**
 * @brief LVGL Display flushing callback
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
 * @brief Touch input reading callback
 */
void my_touchpad_read(lv_indev_t * indev, lv_indev_data_t * data) {
    int val = touchRead(TOUCH_PIN);
    if (touch_base_val - val > TOUCH_THRESHOLD) {
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/**
 * @brief Main game loop timer callback
 */
void game_loop_cb(lv_timer_t * timer) {
    if (!game_running || bird == NULL) return;

    // Apply gravity and update bird position
    bird_vel += GRAVITY;
    bird_y += bird_vel;
    lv_obj_set_y(bird, bird_y);

    // Update pipe position and handle scrolling
    pipe_x -= PIPE_SPEED;
    if (pipe_x < -PIPE_WIDTH) {
        pipe_x = SCREEN_WIDTH;
        pipe_gap_y = 30 + rand() % 80; 
        score++;
        lv_label_set_text_fmt(label_score, "%d", score);
    }
    
    // Refresh pipe UI elements
    if(pipe_top && pipe_bottom) {
        lv_obj_set_x(pipe_top, pipe_x);
        lv_obj_set_x(pipe_bottom, pipe_x);
        lv_obj_set_height(pipe_top, pipe_gap_y);
        lv_obj_set_y(pipe_bottom, pipe_gap_y + PIPE_GAP);
        lv_obj_set_height(pipe_bottom, SCREEN_HEIGHT - (pipe_gap_y + PIPE_GAP));
    }

    // Boundary collision detection
    if (bird_y < 0 || bird_y > (SCREEN_HEIGHT - 20)) {
        game_running = false;
        lv_obj_remove_flag(label_msg, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(label_msg, "CRASHED!\nTap to Restart");
        game_over_action();
    }

    // Pipe collision detection logic
    if (pipe_x < 60 && (pipe_x + PIPE_WIDTH) > 40) { 
        if (bird_y < pipe_gap_y || (bird_y + 18) > (pipe_gap_y + PIPE_GAP)) {
            game_running = false;
            lv_obj_remove_flag(label_msg, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(label_msg, "HIT PIPE!\nTap to Restart");
            game_over_action();
        }
    }
}

/**
 * @brief UI event handler for touch interactions
 */
void event_handler(lv_event_t * e) {
    if (lv_event_get_code(e) == LV_EVENT_PRESSED) {
        if (!game_running) {
            // Reset game state on restart
            bird_y = 100; bird_vel = 0; pipe_x = 240; score = 0;
            lv_label_set_text(label_score, "0");
            lv_obj_add_flag(label_msg, LV_OBJ_FLAG_HIDDEN);
            game_running = true;
        } else {
            bird_vel = -7.5;
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    load_best_score(); 
    Serial.printf("Best score loaded: %d\n", best_score);

    // Hardware Pin Setup
    pinMode(TFT_VCC, OUTPUT);
    digitalWrite(TFT_VCC, LOW);
    delay(100);

    // Display Driver Setup
    tft.begin();
    tft.fillScreen(TFT_BLACK);

    // Touch Calibration
    touchSetCycles(0xf000, 0x1000);
    touch_base_val = touchRead(TOUCH_PIN);

    // LVGL Framework Initialization
    lv_init();
    Serial.println("LVGL Init OK");

    // Display Interface Configuration
    lv_display_t * disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    if(!disp) return;
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, (uint8_t *)buf_aligned, NULL, sizeof(buf_aligned), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Input Device Registration
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_BUTTON);
    lv_indev_set_read_cb(indev, my_touchpad_read);
    static const lv_point_t btn_pts[] = {{120, 120}};
    lv_indev_set_button_points(indev, btn_pts);

    // UI Layout - Background
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x050510), 0);
    lv_obj_t * bg_img = lv_image_create(lv_screen_active());
    lv_image_set_src(bg_img, &img_bg); 
    lv_obj_center(bg_img);

    // Button
    lv_obj_t * bg_btn = lv_button_create(lv_screen_active());
    lv_obj_set_size(bg_btn, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_style_bg_opa(bg_btn, 0, 0);
    lv_obj_set_style_border_opa(bg_btn, 0, 0);
    lv_obj_add_event_cb(bg_btn, event_handler, LV_EVENT_ALL, NULL);

    // UI Layout - Pipes
    pipe_top = lv_obj_create(lv_screen_active());
    lv_obj_set_size(pipe_top, PIPE_WIDTH, 100);
    lv_obj_set_style_bg_color(pipe_top, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_border_color(pipe_top, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_border_width(pipe_top, 2, 0);

    pipe_bottom = lv_obj_create(lv_screen_active());
    lv_obj_set_size(pipe_bottom, PIPE_WIDTH, 100);
    lv_obj_set_style_bg_color(pipe_bottom, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_border_color(pipe_bottom, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_border_width(pipe_bottom, 2, 0);

    // UI Layout - Character (Bird)
    bird = lv_obj_create(lv_screen_active());
    lv_obj_set_size(bird, 18, 18);
    lv_obj_set_style_radius(bird, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(bird, lv_palette_main(LV_PALETTE_PINK), 0);
    lv_obj_set_x(bird, 40);
    lv_obj_set_scrollbar_mode(bird, LV_SCROLLBAR_MODE_OFF);

    // UI Layout - Score
    label_score = lv_label_create(lv_screen_active());
    lv_label_set_text(label_score, "0");
    lv_obj_set_style_text_font(label_score, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(label_score, lv_color_white(), 0);
    lv_obj_set_style_text_opa(label_score, 255, 0);
    lv_obj_align(label_score, LV_ALIGN_TOP_MID, 0, 20);

    // UI Layout - Information
    label_msg = lv_label_create(lv_screen_active());
    lv_label_set_recolor(label_msg, true); 
    lv_obj_set_style_text_align(label_msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text_fmt(label_msg, 
        "#FF3399 CYBER BIRD#\n\n#FF0000 BEST SCORE: %d#\n#000000 Touch to Fly#", 
        best_score);
    lv_obj_set_style_text_color(label_msg, lv_color_white(), 0);
    lv_obj_center(label_msg);

    // Start timer
    lv_timer_create(game_loop_cb, 30, NULL);
    Serial.println("Setup Complete");
}

void loop() {
    // Refresh LVGL tasks
    lv_timer_handler();
    static uint32_t last_tick = 0;
    uint32_t current_ms = millis();
    lv_tick_inc(current_ms - last_tick);
    last_tick = current_ms;
    delay(5);
}