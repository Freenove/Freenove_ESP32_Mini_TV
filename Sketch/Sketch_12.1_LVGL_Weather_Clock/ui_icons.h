#ifndef UI_ICONS_H
#define UI_ICONS_H

#include <lvgl.h>

#define UI_ICON_WEATHER_W   48
#define UI_ICON_WEATHER_H   48
#define UI_ICON_METRIC_W    20
#define UI_ICON_METRIC_H    20

LV_IMAGE_DECLARE(icon_weather_clear);
LV_IMAGE_DECLARE(icon_weather_partly);
LV_IMAGE_DECLARE(icon_weather_cloudy);
LV_IMAGE_DECLARE(icon_weather_fog);
LV_IMAGE_DECLARE(icon_weather_rain);
LV_IMAGE_DECLARE(icon_weather_snow);
LV_IMAGE_DECLARE(icon_weather_thunder);

LV_IMAGE_DECLARE(icon_temp);
LV_IMAGE_DECLARE(icon_humidity);

/** Select weather icon. **/
const lv_image_dsc_t *ui_icon_weather_by_code(int weather_code);

#endif /* UI_ICONS_H */
