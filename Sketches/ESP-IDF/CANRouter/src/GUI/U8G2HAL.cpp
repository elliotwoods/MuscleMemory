#include "U8g2lib.h"
#include "../Devices/I2C.h"
#include "driver/i2c.h"
#include "driver/gpio.h"

// reference : https://github.com/nkolban/esp32-snippets/blob/master/hardware/displays/U8G2/u8g2_esp32_hal.c

#define U8G2_ESP32_HAL_UNDEFINED GPIO_NUM_MAX

i2c_cmd_handle_t i2c_cmd;

struct {
	gpio_num_t reset = GPIO_NUM_16;
	gpio_num_t dc = U8G2_ESP32_HAL_UNDEFINED;
} u8g2_esp32_hal; // Configuration

uint8_t
u8g2_gpio_and_delay_esp32 (u8x8_t * u8x8, uint8_t msg, uint8_t arg_int, void * arg_ptr)
{
	switch(msg) {
	// Initialize the GPIO and DELAY HAL functions.  If the pins for DC and RESET have been
	// specified then we define those pins as GPIO outputs.
		case U8X8_MSG_GPIO_AND_DELAY_INIT: {
			if (u8g2_esp32_hal.dc != U8G2_ESP32_HAL_UNDEFINED) {
				gpio_set_direction(u8g2_esp32_hal.dc
					, gpio_mode_t::GPIO_MODE_OUTPUT);
			}
			if (u8g2_esp32_hal.reset != U8G2_ESP32_HAL_UNDEFINED) {
				gpio_set_direction(u8g2_esp32_hal.reset
					, gpio_mode_t::GPIO_MODE_OUTPUT);
			}
			break;
		}

	// Set the GPIO reset pin to the value passed in through arg_int.
		case U8X8_MSG_GPIO_RESET:
			if (u8g2_esp32_hal.reset != U8G2_ESP32_HAL_UNDEFINED) {
				gpio_set_level(u8g2_esp32_hal.reset, arg_int);
			}
			break;
	// Delay for the number of milliseconds passed in through arg_int.
		case U8X8_MSG_DELAY_MILLI:
			vTaskDelay(arg_int/portTICK_PERIOD_MS);
			break;
	}
	return 0;
}

uint8_t u8g2_byte_hw_i2c_esp32 (u8x8_t * u8x8, uint8_t msg, uint8_t arg_int, void * arg_ptr)
{
	switch(msg) {
		case U8X8_MSG_BYTE_INIT: {
			// The I2C bus is initialised elsewhere
			break;
		}

		case U8X8_MSG_BYTE_SEND: {
			// Modified to use I2C::X()
			uint8_t* data_ptr = (uint8_t*)arg_ptr;
			while( arg_int > 0 ) {
				ESP_ERROR_CHECK(i2c_master_write_byte(i2c_cmd, *data_ptr, true));
				data_ptr++;
				arg_int--;
			}
			break;
		}

		case U8X8_MSG_BYTE_START_TRANSFER: {
			uint8_t i2c_address = u8x8_GetI2CAddress(u8x8);
			i2c_cmd = i2c_cmd_link_create();
			ESP_LOGD(TAG, "Start I2C transfer to %02X.", i2c_address);
			ESP_ERROR_CHECK(i2c_master_start(i2c_cmd));
			ESP_ERROR_CHECK(i2c_master_write_byte(i2c_cmd, i2c_address << 1 | I2C_MASTER_WRITE, true));
			break;
		}

		case U8X8_MSG_BYTE_END_TRANSFER: {
			ESP_LOGD(TAG, "End I2C transfer.");
			ESP_ERROR_CHECK(i2c_master_stop(i2c_cmd));
			Devices::I2C::X().perform(i2c_cmd);
			break;
		}
	}
	return 0;
}