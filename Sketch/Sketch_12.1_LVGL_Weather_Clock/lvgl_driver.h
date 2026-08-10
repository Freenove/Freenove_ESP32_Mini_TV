#ifndef LVGL_DRIVER_H
#define LVGL_DRIVER_H

#include <lvgl.h>

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH  240
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 240
#endif

/** Init panel power, TFT, LVGL display and draw buffer. **/
void lvgl_driver_init(void);
void lvgl_driver_handler(void);

#endif
