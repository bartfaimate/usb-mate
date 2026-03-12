#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H


#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "esp_chip_info.h"
#include "esp_system.h"
#include "esp_private/esp_clk.h"




void print_flash_size(esp_chip_info_t *chip_info);
void print_heap_size();

void print_cpu_freq();

#endif