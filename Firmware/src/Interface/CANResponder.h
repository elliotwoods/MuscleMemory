#pragma once

#include "Control/FilteredTarget.h"

#include <stdint.h>

namespace Interface {
	class CANResponder {
	public:
		CANResponder(Control::FilteredTarget &);
		void init();
		void deinit();
		void update();
	private:
		Control::FilteredTarget & filteredTarget;
		uint16_t deviceIDMask;
	};
}