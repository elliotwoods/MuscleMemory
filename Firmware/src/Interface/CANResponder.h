#pragma once

//#define OTA_ENABLED

#include "Control/FilteredTarget.h"

#ifdef OTA_ENABLED
#include "Interface/CAN/OTAFirmware.h"
#endif

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
#ifdef OTA_ENABLED
		CAN::OTAFirmware otaFirmware;
#endif

		Control::FilteredTarget & filteredTarget;
		uint16_t deviceIDForMask;

		uint16_t rxCount = 0;
		uint16_t txCount = 0;
		uint16_t errorCount = 0;

		uint64_t timeOfLastCANRx = 0;
	};
}