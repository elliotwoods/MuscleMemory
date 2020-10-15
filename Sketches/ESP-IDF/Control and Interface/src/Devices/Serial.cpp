#include "Serial.h"


namespace Devices {
	//---------
	void
	Serial::init(uint16_t baudRate, uart_port_t port)
	{
		uart_config_t uart_config = {0};
		{
			uart_config.baud_rate = baudRate;
			uart_config.data_bits = UART_DATA_8_BITS;
			uart_config.parity    = UART_PARITY_DISABLE;
			uart_config.stop_bits = UART_STOP_BITS_1;
			uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
		}

		{
			auto result = uart_driver_install(port
				, 2048
				, 0
				, 0
				, NULL
				, 0);
			ESP_ERROR_CHECK(result);
		}
		
		{
			auto result = uart_param_config(port, &uart_config);
			ESP_ERROR_CHECK(result);
		}

		{
			auto result = uart_set_pin(port
				, GPIO_NUM_1
				, GPIO_NUM_2
				, UART_PIN_NO_CHANGE
				, UART_PIN_NO_CHANGE);
			ESP_ERROR_CHECK(result);
		}
	}
}