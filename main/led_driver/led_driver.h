#pragma once

#include <stdint.h>
#include <esp_err.h>

void led_driver_init();
void led_driver_set_pixel(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
void led_driver_clear();
esp_err_t led_driver_refresh();
