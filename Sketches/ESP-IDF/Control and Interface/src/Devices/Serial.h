#pragma once

#include "driver/uart.h"

namespace Devices {
	class Serial {
	public:
		void init(uint16_t baudRate, uart_port_t port = uart_port_t::UART_NUM_0);
	};
}