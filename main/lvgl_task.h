#ifndef lvgl_task_h
#define lvgl_task_h


#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/ledc.h"



/********************** PWM *******************/

#define BACKLIGHT_LEDC_TIMER  LEDC_TIMER_0
#define BACKLIGHT_LEDC_MODE   LEDC_LOW_SPEED_MODE
#define BACKLIGHT_LEDC_CH     LEDC_CHANNEL_0
#define BACKLIGHT_LEDC_FREQ   5000   // 5 kHz
#define BACKLIGHT_LEDC_RES    LEDC_TIMER_8_BIT  // 0–255

/********************* I2C *********************/
#define I2C_SCL_IO                  10         /*!< GPIO number used for I2C master clock */
#define I2C_SDA_IO                  11         /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              0         /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000    /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0         /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0         /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

void lvgl_start(void);
void lcd_init(void);

void backlight_init() ;

void set_brightness(uint8_t brightness);

#endif