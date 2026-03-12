#pragma once
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "lvgl.h"

void cst328_lvgl_init(lv_display_t *disp);