#pragma once

#include <stdint.h>

namespace Interface {
	class CANResponder {
	public:
		void init();
		void deinit();
		void update();
	private:
		uint16_t deviceIDMask;
	};
}