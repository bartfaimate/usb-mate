
#include "system_info.h"

#include "esp_flash.h"


void print_flash_size(esp_chip_info_t *chip_info)
{

  uint32_t flash_size;
  if (esp_flash_get_size(NULL, &flash_size) != ESP_OK)
  {
    printf("Get flash size failed");
    return;
  }

  printf("This chip has %" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
         (chip_info->features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
}

void print_heap_size()
{
  printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());
  printf("Minimum free heap size: %" PRIu32 " K bytes\n", esp_get_minimum_free_heap_size() / 1024);
}

void print_cpu_freq()
{
  int freq_mhz = esp_clk_cpu_freq() / 1000000; // 1e6
  printf("CPU frequency: %d MHz\n", freq_mhz);
}