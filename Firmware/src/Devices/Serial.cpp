#include "Serial.h"
#include "Platform/Platform.h"

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
			uart_config.rx_flow_ctrl_thresh = 0;
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
				, MM_CONFIG_SERIAL_PIN_TX
				, MM_CONFIG_SERIAL_PIN_RX
				, UART_PIN_NO_CHANGE
				, UART_PIN_NO_CHANGE);
			ESP_ERROR_CHECK(result);
		}
	}
}