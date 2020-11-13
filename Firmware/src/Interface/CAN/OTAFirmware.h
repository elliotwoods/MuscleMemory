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

#define OTA_BLOCK_SIZE 8

namespace Interface {
	namespace CAN {
		class OTAFirmware {
		public:
			static bool isOTAData(const uint32_t & identifier);
			static bool isOTAInfo(const uint32_t & identifier);

			void update();
			void updateCANTask();
			bool processMessage(const can_message_t &); // Returns true if message was an OTA messaeg
		private:
			void checkBegin(const can_message_t & info);

			bool needsSendRequestData = false;
			bool needsSendRequestInfo = false;
			size_t rxOffset; // The byte offset of the current written data
			size_t totalSize;

			bool isDownloading = false;
			const esp_partition_t * otaPartition;
			esp_ota_handle_t otaHandle;
		};
	}
}