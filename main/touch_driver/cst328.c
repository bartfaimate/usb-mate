#include "cst328.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "lvgl.h"

#define TAG "CST328"

#define I2C_CLK_HZ 400000
#define CST328_I2C_PORT I2C_NUM_0

#define CST328_I2C_ADDR 0x1A
#define CST328_SDA_PIN 1
#define CST328_SCL_PIN 3
#define CST328_RST_PIN 2
#define CST328_INT_PIN 4

#define LCD_H_RES 240
#define LCD_V_RES 320

static volatile bool touch_pending = false;

/* ---------- ISR ---------- */
static void IRAM_ATTR cst328_int_isr(void *arg)
{
  touch_pending = true;
}

/* ---------- I2C ---------- */
static void cst328_i2c_init(void)
{
  i2c_config_t cfg = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = CST328_SDA_PIN,
      .scl_io_num = CST328_SCL_PIN,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master.clk_speed = 400000,
  };

  ESP_ERROR_CHECK(i2c_param_config(CST328_I2C_PORT, &cfg));
  ESP_ERROR_CHECK(i2c_driver_install(CST328_I2C_PORT, cfg.mode, 0, 0, 0));
}

static esp_err_t cst328_read(uint8_t reg, uint8_t *buf, size_t len)
{
  return i2c_master_write_read_device(
      CST328_I2C_PORT,
      CST328_I2C_ADDR,
      &reg, 1,
      buf, len,
      pdMS_TO_TICKS(50));
}

/* ---------- TOUCH ---------- */
static bool cst328_read_xy(uint16_t *x, uint16_t *y)
{
  
  if (!touch_pending)
    return false;

  touch_pending = false;

  uint8_t status;
  if (cst328_read(0x00, &status, 1) != ESP_OK)
    return false;

  // printf("incomming data %d\n", status);

  if (!(status & 0x06))
    return false;

  // reading x cords

  uint8_t buf[4];
  // read 4 bytes
  if (cst328_read(0x01, buf, 4) != ESP_OK)
    return false;

  // printf("got 4 bytes ox%x ox%x ox%x\n", buf[0], buf[1], buf[2]);

  uint16_t rx = ((buf[0]) << 4) | (buf[2] & 0xF0) >> 4;
  uint16_t ry = ((buf[1]) << 4) | (buf[2] & 0x0F);

  // printf("x: %x %d, y: %d\n", rx, rx, ry);

  if (rx < 0 || rx > LCD_H_RES || ry < 0 || ry > LCD_V_RES)
    return false;

  *x = rx;
  *y = ry;

  return true;
}

/* ---------- LVGL ---------- */
static void lvgl_touch_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
  uint16_t x, y;

  if (cst328_read_xy(&x, &y))
  {
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = x;
    data->point.y = y;
  }
  else
  {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

/* ---------- INIT ---------- */
void cst328_lvgl_init(lv_display_t *disp)
{
  /* I2C */
  cst328_i2c_init();

  /* INT pin */
  gpio_config_t io_cfg = {
      .pin_bit_mask = 1ULL << CST328_INT_PIN,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .intr_type = GPIO_INTR_NEGEDGE,
  };
  gpio_config(&io_cfg);

  gpio_install_isr_service(0);
  gpio_isr_handler_add(CST328_INT_PIN, cst328_int_isr, NULL);

  /* LVGL input */
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, lvgl_touch_cb);

  ESP_LOGI(TAG, "CST328 touch initialized (INT driven)");
}