/*
* @ File:   Sketch_09.1_LVGL_Picture.ino
* @ Author: [Vegetable-SYC]
* @ Date:   [2025-12-19]
*/

#include <lvgl.h>
#include <TFT_eSPI.h>
#include <image.h>

/* Display and asset dimensions */
#define IMG_W  240
#define IMG_H  240
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 240

/* Hardware pin configuration */
const int TFT_VCC = 21;
TFT_eSPI tft = TFT_eSPI(); 

/* LVGL rendering buffer */
static uint8_t draw_buf[SCREEN_WIDTH * SCREEN_HEIGHT / 10 * 2]; 

/* LVGL Image Descriptor mapping the C-array asset */
const lv_image_dsc_t lv_img = {
    .header = {
        .magic = LV_IMAGE_HEADER_MAGIC,
        .cf = LV_COLOR_FORMAT_RGB565, // Color format: 16-bit RGB     
        .flags = 0,
        .w = IMG_W,
        .h = IMG_H,
        .stride = IMG_W * 2,          // Bytes per line          
        .reserved_2 = 0
    },
    .data_size = IMG_W * IMG_H * 2,        
    .data = img_asset,                // Pointer to external image data
};

/**
 * Display flushing callback: Transfers pixels from LVGL to hardware
 */
void my_disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)px_map, w * h, true);
    tft.endWrite();

    lv_display_flush_ready(disp); // Inform library flushing is complete
}

void setup() {
    Serial.begin(115200);

    /* Hardware and library initialization */
    tft.begin();
    lv_init();

    /* Create and configure display abstraction layer */
    lv_display_t * disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Initialize UI components: Image display */
    lv_obj_t * img_obj = lv_image_create(lv_screen_active());
    lv_image_set_src(img_obj, &lv_img);
    lv_obj_center(img_obj);

    /* Status label configuration */
    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "LVGL v9 Image Display");
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);

    /* Enable display power */
    pinMode(TFT_VCC, OUTPUT);
    digitalWrite(TFT_VCC, LOW); 

    Serial.println("System initialized: LVGL asset display ready.");
}

void loop() {
    /* Process timer events and system task handlers */
    lv_timer_handler();

    /* Advance internal library tick counts */
    static uint32_t last_tick = 0;
    uint32_t current_ms = millis();
    lv_tick_inc(current_ms - last_tick);
    last_tick = current_ms;

    delay(5);
}