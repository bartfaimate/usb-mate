#include "esp_chip_info.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <inttypes.h>
#include <stdio.h>

#include "system_info.h"

#include "lvgl_task.h"

#define MAIN_TASK_STACK 1024 * 12
#define MAIN_TASK_CORE_ID 0
#define MAIN_TASK_PRIO 5

void driver_init() {
  esp_chip_info_t chip_info;

  esp_chip_info(&chip_info);
  printf("This is %s chip with %d CPU core(s), %s%s%s%s\n", CONFIG_IDF_TARGET,
         chip_info.cores,
         (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
         (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
         (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
         (chip_info.features & CHIP_FEATURE_IEEE802154)
             ? ", 802.15.4 (Zigbee/Thread)"
             : "");

  unsigned major_rev = chip_info.revision / 100;
  unsigned minor_rev = chip_info.revision % 100;
  printf("silicon revision v%d.%d, ", major_rev, minor_rev);

  print_flash_size(&chip_info);
  print_heap_size();
  print_cpu_freq();

  // ESP_ERROR_CHECK(nvs_flash_init());

  return;
}

void main_task(void *arg) {
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(100));
    // do something
    // read input
    // calculate
    // render
  }
}

void app_main(void) {

  driver_init();

  lvgl_start();

  xTaskCreatePinnedToCore(main_task, "main", MAIN_TASK_STACK, NULL,
                          MAIN_TASK_PRIO, NULL, MAIN_TASK_CORE_ID);

  vTaskDelete(NULL); // VERY IMPORTANT
}