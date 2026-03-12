#include "lvgl_task.h"
#include "lvgl.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "driver/gpio.h"
#include "touch_driver/cst328.h"

#include "ui.h"

#include "ui_events.h"


#define TAG "LVGL_INIT"

/* Display resolution */
#define LCD_H_RES 240
#define LCD_V_RES 320
#define LCD_DISPLAY "ST7789"

#define SWAP_BYTES 1 // change for big endian vs little endian

/* SPI pins */
#define PIN_NUM_LCD_MOSI 45
#define PIN_NUM_LCD_MISO -1
#define PIN_NUM_LCD_CLK 40
#define PIN_NUM_LCD_CS 42
#define PIN_NUM_LCD_DC 41
#define PIN_NUM_LCD_RST 39
#define PIN_NUM_LCD_BACK_LIGHT 5

#define LCD_HOST SPI2_HOST
#define LCD_SPI_CLK_HZ  80000000

/* LVGL Task Settings */
#define LVGL_TASK_STACK (1024 * 12)
#define LVGL_TASK_PRIO 5
#define LVGL_CORE_ID 1

/* Draw Buffer - 1/8th of screen size is usually enough and fits in internal RAM */
#define DRAW_BUF_LINES 40
#define DRAW_BUF_SIZE (LCD_H_RES * DRAW_BUF_LINES * sizeof(lv_color16_t))

QueueHandle_t ui_event_queue = NULL;

/* Memory alignment is critical for DMA */
static uint8_t buf1[DRAW_BUF_SIZE] LV_ATTRIBUTE_MEM_ALIGN;

static esp_lcd_panel_handle_t panel_handle;
static lv_display_t *s_disp = NULL;

/* ---------------- CALLBACKS ---------------- */
/*
 * LVGL tick callback 
 */
static void lv_tick_cb(void *arg)
{
  lv_tick_inc(1);
}

static bool lcd_flush_ready_cb(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
  if (s_disp)
  {
    lv_display_flush_ready(s_disp);
  }
  return false;
}

/**
 * LCD Flush callback. Swaps bytes if enabled and puts data to LCD
 */
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
#if !SWAP_BYTES

  esp_err_t res;
  /* Copy the pixel data to the LCD controller */
  res = esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, px_map);

  if(res != ESP_OK) { ESP_LOGE(TAG, "Something is wrong\n");}
#else
  uint32_t px_count =
        (area->x2 - area->x1 + 1) *
        (area->y2 - area->y1 + 1);

    uint16_t *buf = (uint16_t *)px_map;

    for (uint32_t i = 0; i < px_count; i++) {
        uint16_t c = buf[i];
        buf[i] = (c >> 8) | (c << 8);
    }

    esp_lcd_panel_draw_bitmap(
        panel_handle,
        area->x1, area->y1,
        area->x2 + 1, area->y2 + 1,
        buf
    );
#endif
}

/* ---------------- TASK ---------------- */
extern QueueHandle_t ui_event_queue;

/**
 *  this is the main UI task
 *  it handles all kind of events
 */
static void lvgl_task(void *arg)
{
  ESP_LOGI(TAG, "Starting LVGL main loop");
  // init user interface
  init_gui();  
}

/* ---------------- INIT ---------------- */

/**
 * Initialise LCD display
 * init SPI bus for LCD
 * init touch controls
 */
void lcd_init(void)
{
  /* 1. SPI Bus Initialization */
  spi_bus_config_t buscfg = {
      .mosi_io_num = PIN_NUM_LCD_MOSI,
      .miso_io_num = PIN_NUM_LCD_MISO,
      .sclk_io_num = PIN_NUM_LCD_CLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = DRAW_BUF_SIZE,
  };
  ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

  /* 2. LCD Panel IO Configuration */
  esp_lcd_panel_io_handle_t io_handle = NULL;
  esp_lcd_panel_io_spi_config_t io_cfg = {
      .dc_gpio_num = PIN_NUM_LCD_DC,
      .cs_gpio_num = PIN_NUM_LCD_CS,
      .pclk_hz = LCD_SPI_CLK_HZ,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
      
      .spi_mode = 0,
      .trans_queue_depth = 10,
      .on_color_trans_done = lcd_flush_ready_cb,
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_HOST, &io_cfg, &io_handle));

  /* 3. Install Display Driver (ST7789) */
  esp_lcd_panel_dev_config_t panel_cfg = {
      .reset_gpio_num = PIN_NUM_LCD_RST,
      .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
      .bits_per_pixel = 16,
  };
  ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_cfg, &panel_handle));

  ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

  backlight_init();
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
}

/**
 * start the LVGL process
 * init LCD, init lvgl module
 * 
 */
void lvgl_start(void)
{
  /* init LCD */
  lcd_init();

  /*  Initialize LVGL */
  lv_init();

  /* Create Display */
  s_disp = lv_display_create(LCD_H_RES, LCD_V_RES);

  /* Configure the draw buffer */
  // lv_display_set_color_format(s_disp, LV_COLOR_FORMAT_RGB565_SWAPPED);
  lv_display_set_buffers(s_disp, buf1, NULL, DRAW_BUF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(s_disp, lvgl_flush_cb);

  /* init touch */
  cst328_lvgl_init(s_disp);

  /* 5. Set up LVGL Tick Timer */
  const esp_timer_create_args_t tick_args = {
      .callback = lv_tick_cb,
      .name = "lvgl_tick"};
  esp_timer_handle_t tick_timer;
  ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, 1000));

  ui_event_queue = xQueueCreate(8, sizeof(ui_event_t));
  assert(ui_event_queue);

  /* 6. Launch LVGL Task */
  xTaskCreatePinnedToCore(lvgl_task, "lvgl", LVGL_TASK_STACK, NULL, LVGL_TASK_PRIO, NULL, LVGL_CORE_ID);

  ESP_LOGI(TAG, "LVGL successfully initialized");
}

void backlight_init() {

    ledc_timer_config_t timer_conf = {
        .speed_mode       = BACKLIGHT_LEDC_MODE,
        .timer_num        = BACKLIGHT_LEDC_TIMER,
        .duty_resolution  = BACKLIGHT_LEDC_RES,
        .freq_hz          = BACKLIGHT_LEDC_FREQ,
        .clk_cfg          = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    ledc_channel_config_t channel_conf = {
        .gpio_num   = PIN_NUM_LCD_BACK_LIGHT,
        .speed_mode = BACKLIGHT_LEDC_MODE,
        .channel    = BACKLIGHT_LEDC_CH,
        .timer_sel  = BACKLIGHT_LEDC_TIMER,
        .duty       = 255,   // start at full brightness
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));

}

/**
 * set brightness callback
 */
void set_brightness(uint8_t brightness) {
  if (brightness > 100) brightness = 100;

    // Map 0–100% → 0–255 duty
    uint32_t duty = (brightness * 255) / 100;

    ledc_set_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CH, duty);
    ledc_update_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CH);

}