#include "OTAFirmware.h"
#include "Registry.h"

template<typename T>
const T & readAndMove(const uint8_t* & dataMover)
{
	auto & value = * (T*) dataMover;
	dataMover += sizeof(T);
	return value;
}

template<typename T>
T & valueAndMove(uint8_t* & dataMover)
{
	auto & value = * (T*) dataMover;
	dataMover += sizeof(T);
	return value;
}

// MESSAGE FORMAT
// 18th bit of ID is 1 for an OTA data packet
// bits 0-18 are all 1 for an OTA info packet
// Remaining 17 bits are the block address (0-131072)
// Blocks are 8 bytes
// i.e. we only support FW that is smaller than 1MB for now

namespace Interface {
	namespace CAN {
		//----------
		bool
		OTAFirmware::isOTAData(const uint32_t & identifier)
		{
			return (identifier & (1 << 17)) > 0;
		}

		//----------
		bool
		OTAFirmware::isOTAInfo(const uint32_t & identifier)
		{
			return (identifier & ((1 << 18) - 1)) > 0;
		}

		//----------
		void
		OTAFirmware::update()
		{
			setRegisterValue(Registry::RegisterType::OTADownloading, this->isDownloading ? 0 : 1);
			setRegisterValue(Registry::RegisterType::OTARxPosition, this->rxOffset);
		}

		//----------
		void
		OTAFirmware::updateCANTask()
		{
			if(this->needsSendRequestInfo) {
				can_message_t message;
				message.identifier = getRegisterValue(Registry::RegisterType::DeviceID);
				auto data = message.data;

				valueAndMove<Registry::Operation>(data) = Registry::Operation::OTARequestInfo;
				message.data_length_code = sizeof(Registry::Operation);

				can_transmit(&message, 10 / portTICK_PERIOD_MS);

				this->needsSendRequestInfo = false;
			}

			if(this->needsSendRequestData) {
				can_message_t message;
				message.identifier = getRegisterValue(Registry::RegisterType::DeviceID);
				auto data = message.data;

				valueAndMove<size_t>(data) = this->rxOffset;
				message.data_length_code = sizeof(size_t);

				can_transmit(&message, 10 / portTICK_PERIOD_MS);

				this->needsSendRequestData = false;
			}
		}

		//----------
		bool
		OTAFirmware::processMessage(const can_message_t & message)
		{
			if(isOTAInfo(message.identifier)) {
				this->checkBegin(message);
				return true;
			}
			else if(isOTAData(message.identifier)) {
				if(this->isDownloading) {
					size_t blockAddress = message.identifier & ((1 << 17) - 1);
					size_t writeOffset = blockAddress * OTA_BLOCK_SIZE;
					
					// Check if the writeOffset is already past our buffer write position
					if(writeOffset > this->rxOffset) {
						// The sender has gone past our buffer position
						this->needsSendRequestData = true;
					}
					else {
						ESP_ERROR_CHECK(esp_ota_write(this->otaHandle
							, message.data
							, OTA_BLOCK_SIZE));
						if(writeOffset + OTA_BLOCK_SIZE >= this->totalSize) {
							// this is the last packet
							ESP_ERROR_CHECK(esp_ota_end(this->otaHandle));

							// check if same as what we already ahve
							if(esp_partition_check_identity(esp_ota_get_running_partition(), this->otaPartition)) {
								printf("[OTA] : Ignoring firmware since it is same as current\n");
							}
							else {
								// set the boot partition to the new firmware
								ESP_ERROR_CHECK(esp_ota_set_boot_partition(this->otaPartition));

								printf("[OTA] Firmware updated, rebooting now...");
								esp_restart();
							}
							
							this->isDownloading = false;
						}
					}
				}
				else {
					// We don't have the info of the update
					this->needsSendRequestInfo = true;
				}
				return true;
			}

			// This is not an OTA message
			return false;
		}

		//----------
		void
		OTAFirmware::checkBegin(const can_message_t & message)
		{
			auto data = message.data;
			auto totalSize = readAndMove<uint32_t>(data);

			if(this->isDownloading && totalSize == this->totalSize) {
				// ignore this if already downloading
				return;
			}

			this->totalSize = totalSize;
			this->rxOffset = 0;
			this->isDownloading = true;

			this->otaPartition = esp_ota_get_next_update_partition(nullptr);
			esp_ota_begin(this->otaPartition
				, OTA_SIZE_UNKNOWN
				, &this->otaHandle);

			printf("[OTA] Beginning OTA download with size (%d)\n", totalSize);
		}
	}
}