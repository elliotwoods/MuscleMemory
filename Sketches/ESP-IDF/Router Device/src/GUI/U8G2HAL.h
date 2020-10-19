#pragma once
#include "U8g2lib.h"

uint8_t u8g2_gpio_and_delay_esp32 (u8x8_t * u8x8, uint8_t msg, uint8_t arg_int, void * arg_ptr);
uint8_t u8g2_byte_hw_i2c_esp32 (u8x8_t * u8x8, uint8_t msg, uint8_t arg_int, void * arg_ptr);