#pragma once

//#define OTA_ENABLED

#include "Control/FilteredTarget.h"
#include "Registry.h"

#ifdef OTA_ENABLED
#include "Interface/CAN/OTAFirmware.h"
#endif

#include <stdint.h>
#include <vector>

extern "C" {
	#include "driver/can.h"
}

namespace Interface {
	class CANResponder {
	public:
		CANResponder();
		void init();
		void deinit();
		void update(); // From main loop
		void updateTask(); // From CAN task
		void notifyKeepAlive();
	private:
		void queuePingResponse();
		void queueReadRequest(Registry::RegisterType);
		bool receive(TickType_t timeout);
		void send();
		std::vector<can_message_t> txQueue;
#ifdef OTA_ENABLED
		CAN::OTAFirmware otaFirmware;
#endif

		uint16_t deviceIDForMask;

		uint16_t rxCount = 0;
		uint16_t txCount = 0;
		uint16_t errorCount = 0;

		uint64_t timeOfLastCANRx = 0;
	};
}