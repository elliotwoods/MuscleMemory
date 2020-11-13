#pragma once

#ifdef ARDUINO
#include "FreeRTOS.h"
#else
#include <freertos/FreeRTOS.h>
#endif


extern "C" {
	#include "driver/gpio.h"
	#include "driver/can.h"
	#include "esp_ota_ops.h"
}

#define OTA_BUFFER_SIZE 0x1000

namespace Interface {
	namespace CAN {
		class OTAFirmware {
		public:
			static bool isOTAData(const uint32_t & identifier);
			static bool isOTAInfo(const uint32_t & identifier);

			void update();
			void updateCANTask();
			void processMessage(const can_message_t &); // Returns true if message was an OTA messaeg
		private:
			void begin(size_t size);

			bool needsSendRequestData = false;
			bool needsSendRequestInfo = false;

			size_t writePosition = 0; // The byte offset of the current written data
			size_t bufferPosition = 0;
			size_t size = 0;

			uint8_t * buffer = nullptr;

			const esp_partition_t * otaPartition;
			esp_ota_handle_t otaHandle;
		};
	}
}