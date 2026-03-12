#ifndef ui_h
#define ui_h

/*********************************************
* All UI related components and handlers
*********************************************/

#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#define TAG "LVGL_INIT"

/* Display resolution */
#define LCD_H_RES 240
#define LCD_V_RES 320


void init_gui(void);



#endif