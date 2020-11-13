#pragma once

#include "Control/FilteredTarget.h"
#include "Interface/CAN/OTAFirmware.h"

#include <stdint.h>

namespace Interface {
	class CANResponder {
	public:
		CANResponder(Control::FilteredTarget &);
		void init();
		void deinit();
		void update(); // From main loop
		void updateTask(); // From CAN task
	private:
		CAN::OTAFirmware otaFirmware;
		
		Control::FilteredTarget & filteredTarget;
		uint16_t deviceIDForMask;

		uint16_t rxCount = 0;
		uint16_t txCount = 0;
		uint16_t errorCount = 0;
	};
}