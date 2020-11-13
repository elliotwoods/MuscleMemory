#include "OTAFirmware.h"
#include "Registry.h"

#include "GUI/Controller.h"
#include "GUI/Panels/OTADownload.h"

#include "esp_task_wdt.h"

#include "ArduinoOTA.h"

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
// uint32_t offset, 4 bytes of data

namespace Interface {
	namespace CAN {
		//----------
		void
		OTAFirmware::update()
		{
			setRegisterValue(Registry::RegisterType::OTAWritePosition, this->writePosition + this->bufferPosition);
			setRegisterValue(Registry::RegisterType::OTASize, this->size);
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

				valueAndMove<Registry::Operation>(data) = Registry::Operation::OTARequestData;
				valueAndMove<size_t>(data) = this->writePosition + this->bufferPosition;
				message.data_length_code = sizeof(size_t) + sizeof(Registry::Operation);

				can_transmit(&message, 10 / portTICK_PERIOD_MS);

				this->needsSendRequestData = false;
			}
		}

		//----------
		void
		OTAFirmware::processMessage(const can_message_t & message)
		{
			auto data = message.data;
			auto operation = readAndMove<Registry::Operation>(data);

			if(operation == Registry::Operation::OTAInfo) {
				auto totalSize = readAndMove<uint32_t>(data);
				this->begin(totalSize);
			}
			else if(operation == Registry::Operation::OTAData) {
				if(getRegisterValue(Registry::RegisterType::OTADownloading) == 0) {
					// We're not downloading, request the info so we can start
					this->needsSendRequestInfo = true;
				}
				else {
					// We are downloading, procces...
					auto incomingOffset = 0;
					incomingOffset += readAndMove<uint8_t>(data);
					incomingOffset += readAndMove<uint8_t>(data) << 8;
					incomingOffset += readAndMove<uint8_t>(data) << 16;

					//printf("Incoming (%d) %02X %02X %02X %02X\n", incomingOffset, message.data[4],message.data[5],message.data[6],message.data[7]);
					
					if(incomingOffset > this->writePosition + this->bufferPosition) {
						// The sender is ahead of us
						this->needsSendRequestData = true;
						printf("[OTA] : Incoming data is ahead (%d) of our current rxPosition (%d). Slow the sending down\n"
							, incomingOffset
							, this->writePosition + this->bufferPosition);
					}
					else if(incomingOffset == this->writePosition + this->bufferPosition) {
						// Copy into buffer
						memcpy(this->buffer + this->bufferPosition
							, message.data + 4
							, 4);
						this->bufferPosition += 4;
						
						if(this->bufferPosition == OTA_BUFFER_SIZE) {
							ESP_ERROR_CHECK(esp_ota_write(this->otaHandle
								, this->buffer
								, OTA_BUFFER_SIZE));
							this->bufferPosition = 0;
							this->writePosition += OTA_BUFFER_SIZE;

							if(this->writePosition >= this->size) {
								// this is the last packet
								ESP_ERROR_CHECK(esp_ota_end(this->otaHandle));

								// check if same as what we already ahve
								if(esp_partition_check_identity(esp_ota_get_running_partition(), this->otaPartition)) {
									printf("[OTA] : Ignoring firmware since it is same as current\n");
								}
								else {
									// set the boot partition to the new firmware
									ESP_ERROR_CHECK(esp_ota_set_boot_partition(this->otaPartition));

									printf("[OTA] Firmware updated, rebooting now...\n");
									esp_restart();
								}
								
								setRegisterValue(Registry::RegisterType::OTADownloading, 0);
							}
						}
					}
				}
			}
		}

		//----------
		void
		OTAFirmware::begin(size_t size)
		{
			if(getRegisterValue(Registry::RegisterType::OTADownloading) == 1 && size == this->size) {
				// ignore this if already downloading
				return;
			}

			this->size = size;
			this->writePosition = 0;
			setRegisterValue(Registry::RegisterType::OTADownloading, 1);
			this->bufferPosition = 0;

			// Allocate buffer
			if(this->buffer) {
				delete[] this->buffer;
			}
			this->buffer = new uint8_t[OTA_BUFFER_SIZE];

			this->otaPartition = esp_ota_get_next_update_partition(nullptr);
			printf("[OTA] Beginning OTA download with size (%d)\n", size);

			printf("[OTA] Allocating OTA download to partition (%s)\n", this->otaPartition->label);

			GUI::Controller::X().pushPanel(std::make_shared<GUI::Panels::OTADownload>());
			this->update();
			GUI::Controller::X().update();

			disableCore0WDT();

			esp_ota_begin(this->otaPartition
				, OTA_SIZE_UNKNOWN
				, &this->otaHandle);

			enableCore0WDT();

			printf("[OTA] Ready...\n");

		}
	}
}