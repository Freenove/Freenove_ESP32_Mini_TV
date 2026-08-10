#ifndef UI_FONT_CLOCK_H
#define UI_FONT_CLOCK_H

#include <lvgl.h>

#ifndef UI_CLOCK_USE_CUSTOM_FONT
#define UI_CLOCK_USE_CUSTOM_FONT 1
#endif

#if UI_CLOCK_USE_CUSTOM_FONT
LV_FONT_DECLARE(ui_font_dseg7_48);
LV_FONT_DECLARE(ui_font_dseg7_22);
#define UI_FONT_CLOCK_LG  (&ui_font_dseg7_48)
#define UI_FONT_CLOCK_SM  (&ui_font_dseg7_22)
#endif

#endif
