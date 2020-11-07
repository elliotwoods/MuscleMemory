#pragma once

#include "driver/uart.h"

#ifdef ARDUINO
	#define UART_DEFAULT_PORT uart_port_t::UART_NUM_0
#else
	#define UART_DEFAULT_PORT 0
#endif

namespace Devices {
	class Serial {
	public:
		void init(uint16_t baudRate, uart_port_t port = UART_DEFAULT_PORT);
	};
}