#include "CANResponder.h"
#include "Registry.h"

#ifdef ARDUINO
#include "FreeRTOS.h"
#else
#include <freertos/FreeRTOS.h>
#endif

#include "esp_log.h"
#include "esp_attr.h"

#include <set>

#define CAN_PRINT_PREVIEW_ENABLED false
#define CAN_ENABLE_LIVE_DEVICE_ID_SWITCH false

#define ERR_DELAY_US                    (800 * 500/25)     //Approximate time for arbitration phase at 500KBPS
#define ERR_PERIOD_US                   (80 * 500/25)      //Approximate time for two bits at 500KBPS

#define TX_GPIO_NUM GPIO_NUM_21
#define RX_GPIO_NUM GPIO_NUM_22

#define MOD_TAG "CAN"

// Reference for alerts and recovery : https://github.com/AlexRogalskiy/nodemcu-test-flight/blob/b3aa90a1dec3dd9dc2982efe9c8700054b30ed4d/esp-idf/peripherals/twai/twai_alert_and_recovery/main/twai_alert_and_recovery_example_main.c

extern "C" {
	#include "driver/gpio.h"
	#include "driver/can.h"
}

template<typename T>
T & valueAndMove(uint8_t* & dataMover)
{
	auto & value = * (T*) dataMover;
	dataMover += sizeof(T);
	return value;
}

namespace Interface {
	//----------
	CANResponder::CANResponder()
	{

	}

	//----------
	void
	CANResponder::init()
	{
		// set CAN ----------------------------------------------------------
		//Initialize configuration structures using macro initializers
		// NOTE : this will fail to compile
		// We change line 108 of can.h to define CAN_IO_UNUSED to be GPIO_NUM_MAX
		can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, CAN_MODE_NORMAL);
		{
			g_config.rx_queue_len = 64;
			g_config.tx_queue_len = 64;
		}

		can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();

		can_filter_config_t f_config;
		{
			this->deviceIDForMask = (uint16_t) getRegisterValue(Registry::RegisterType::DeviceID);
			f_config.acceptance_code = this->deviceIDForMask << (29 - 10 - 13); // Mask is written from 13th bit. ID is stored in 29-10th bit
			f_config.acceptance_code |= (1023 << (29 - 10 - 13)) << 16; // also accept 1023 
			f_config.acceptance_mask = 0b00000000001111110000000000111111; // Accept 1023 and our actual device ID
			f_config.single_filter = false;
		}

		//Install & Start CAN driver
		ESP_ERROR_CHECK(can_driver_install(&g_config, &t_config, &f_config));
		ESP_ERROR_CHECK(can_start());

		//Setup bus alerts
		ESP_ERROR_CHECK(can_reconfigure_alerts(CAN_ALERT_ABOVE_ERR_WARN | CAN_ALERT_ERR_PASS | CAN_ALERT_BUS_OFF, NULL));

		this->timeOfLastCANRx = esp_timer_get_time();
	}

	//----------
	void
	CANResponder::deinit()
	{
		ESP_ERROR_CHECK(can_stop());
		
		// Flush the Rx queue
		{
			can_message_t message;
			while(can_receive(&message, 0) == ESP_OK) {

			}
		}

		ESP_ERROR_CHECK(can_driver_uninstall());
	}

	//----------
	void
	CANResponder::update()
	{
#ifdef OTA_ENABLED
		this->otaFirmware.update();
#endif

		if (CAN_PRINT_PREVIEW_ENABLED) {
			if(this->rxCount > 0 || this->txCount > 0 || this->errorCount > 0) {
				printf("[CAN] Rx : (%u), Tx : (%u), Errors : (%u)\n", this->rxCount, this->txCount, this->errorCount);
			}
		}

		// Watchdog timer
		{
			auto now = esp_timer_get_time();
			if(getRegisterValue(Registry::RegisterType::CANWatchdogEnabled) == 1) {
				auto timeSinceLastMessage = (int) ((now - this->timeOfLastCANRx) / 1000);
				if(timeSinceLastMessage > getRegisterValue(Registry::RegisterType::CANWatchdogTimeout)) {
					printf("[CANResponder] Watchdog timeout on messages received. Rebooting.\n");
					esp_restart();
				}
				setRegisterValue(Registry::RegisterType::CANWatchdogTimer, timeSinceLastMessage);
			}
			if(this->rxCount > 0) {
				this->timeOfLastCANRx = now;
			}
		}

		// Check if Device ID changed
		{
			auto deviceIDForMask = (uint16_t) getRegisterValue(Registry::RegisterType::DeviceID);
			if(deviceIDForMask != this->deviceIDForMask) {
				if(CAN_ENABLE_LIVE_DEVICE_ID_SWITCH) {
					this->deinit();
					this->init();
				}
				else {
					printf("Device ID has changed. Make sure default is saved and then reboot\n");
				}
			}
		}

		// Increment total errors
		if(this->errorCount > 0) {
			auto canErrorsTotal = getRegisterValue(Registry::RegisterType::CANErrorsTotal);
			canErrorsTotal += this->errorCount;
			setRegisterValue(Registry::RegisterType::CANErrorsTotal, canErrorsTotal);
		}

		// Save registers
		setRegisterValue(Registry::RegisterType::CANRxThisFrame, this->rxCount);
		setRegisterValue(Registry::RegisterType::CANTxThisFrame, this->txCount);
		setRegisterValue(Registry::RegisterType::CANErrorsThisFrame, this->errorCount);

		// Blank for next frame
		this->rxCount = 0;
		this->txCount = 0;
		this->errorCount = 0;
	}

	//----------
	void IRAM_ATTR
	CANResponder::updateTask()
	{
#ifdef OTA_ENABLED
		this->otaFirmware.updateCANTask();
#endif

		// handle incoming messages
		while(this->receive(10 / portTICK_PERIOD_MS)) {

		}

		// handle outgoing messages
		this->send();

		// Read alerts
		{
			uint32_t alerts;
			while(can_read_alerts(&alerts, 0) == ESP_OK) {
				if (alerts & CAN_ALERT_ABOVE_ERR_WARN) {
					ESP_LOGI(MOD_TAG, "Surpassed Error Warning Limit");
					this->errorCount++;
				}
				if (alerts & CAN_ALERT_ERR_PASS) {
					ESP_LOGI(MOD_TAG, "Entered Error Passive state");
					this->errorCount++;
				}
				if (alerts & CAN_ALERT_BUS_ERROR) {
					ESP_LOGI(MOD_TAG, "Bus Off state");
					//Prepare to initiate bus recovery, reconfigure alerts to detect bus recovery completion
					can_reconfigure_alerts(CAN_ALERT_BUS_RECOVERED, NULL);
					for (int i = 3; i > 0; i--) {
						ESP_LOGW(MOD_TAG, "Initiate bus recovery in %d", i);
						vTaskDelay(pdMS_TO_TICKS(1000));
					}
					can_initiate_recovery();    //Needs 128 occurrences of bus free signal
					ESP_LOGI(MOD_TAG, "Initiate bus recovery");
					this->errorCount++;
				}
				if (alerts & CAN_ALERT_BUS_RECOVERED) {
					//Bus recovery was successful, exit control task to uninstall driver
					ESP_LOGI(MOD_TAG, "Bus Recovered");
					this->deinit();
					this->init();
					break;
				}
			}
		}
	}

	//----------
	void
	CANResponder::queueReadRequest(Registry::RegisterType registerType)
	{
		static auto & registry = Registry::X();

		auto findRegister = registry.registers.find(registerType);
		if(findRegister == registry.registers.end()) {
			printf("[CAN] : Error on read request. Register (%d) not found\n", (uint16_t) registerType);
		}
		else {
			can_message_t message;
			message.flags = CAN_MSG_FLAG_EXTD;
			message.identifier = getRegisterValue(Registry::RegisterType::DeviceID) << 19;

			auto data = message.data;
			valueAndMove<Registry::Operation>(data) = Registry::Operation::ReadResponse;
			valueAndMove<Registry::RegisterType>(data) = registerType;
			valueAndMove<int32_t>(data) = findRegister->second.value;

			message.data_length_code = sizeof(Registry::Operation) + sizeof(Registry::RegisterType) + sizeof(int32_t);

			if (CAN_PRINT_PREVIEW_ENABLED) {
				printf("Can Tx. ID [%d], Flags [%d], Data [%X %X %X %X %X %X %X %X (%d)]\n"
					, message.identifier
					, message.flags
					, message.data[0]
					, message.data[1]
					, message.data[2]
					, message.data[3]
					, message.data[4]
					, message.data[5]
					, message.data[6]
					, message.data[7]
					, message.data_length_code
					);
			}

			this->txQueue.push_back(message);
		}
	}

	//----------
	bool
	CANResponder::receive(TickType_t timeout)
	{
		static auto & registry = Registry::X();

		can_message_t message;

		if(!can_receive(&message, timeout) == ESP_OK) {
			return false;
		}
		else {
			this->rxCount++;

			auto dataMover = message.data;
			auto operation = valueAndMove<Registry::Operation>(dataMover);

			if(operation >= Registry::Operation::OTARequests) {
#ifdef OTA_ENABLED
				this->otaFirmware.processMessage(message);
				this->rxCount++;
#endif
			}
			else {
				auto registerID = valueAndMove<Registry::RegisterType>(dataMover);
				auto value = valueAndMove<int32_t>(dataMover);

				// Print message preview
				if (CAN_PRINT_PREVIEW_ENABLED) {
					printf("Can Rx. ID [%d], Flags [%d], Data [%X %X %X %X %X %X %X %X (%d)]\n"
						, message.identifier
						, message.flags
						, message.data[0]
						, message.data[1]
						, message.data[2]
						, message.data[3]
						, message.data[4]
						, message.data[5]
						, message.data[6]
						, message.data[7]
						, message.data_length_code
						);

					char registerName[100];
					{
						auto findRegister = registry.registers.find(registerID);
						if(findRegister == registry.registers.end()) {
							sprintf(registerName, "Unknown (%d)", (uint16_t) registerID);
						}
						else {
							sprintf(registerName, "%s", findRegister->second.name.c_str());
						}
					}
					printf("Operation [%d], Register [%s], Value [%d]\n"
						, (uint8_t) operation
						, registerName
						, value);
				}

				if(message.flags & CAN_MSG_FLAG_RTR) {
					// Queue full read response
					if (CAN_PRINT_PREVIEW_ENABLED) {
						printf("RETR\n");
					}
					for(const auto & defaultRegisterType : Registry::defaultRegisterReads) {
						this->queueReadRequest(defaultRegisterType);
					}
				}
				else if(operation == Registry::Operation::ReadRequest) {
					// Queue individual read response
					this->queueReadRequest(registerID);
				}
				else if(operation == Registry::Operation::WriteRequest) {
					// Perform write requests
					if(message.data_length_code == sizeof(Registry::Operation) + sizeof(Registry::RegisterType) + sizeof(int32_t)) {
						auto findRegister = registry.registers.find(registerID);
						if(findRegister == registry.registers.end()) {
							printf("[CAN] : Error on write request. Register (%d) not found", (uint16_t) registerID);
						}
						else {
							findRegister->second.value = value;
							if(registerID == Registry::RegisterType::TargetPosition) {
								Control::FilteredTarget::X().notifyTargetChange();
							}
						}
					}
				}
				else if(operation == Registry::Operation::WriteDefault) {
					// Perform write default requests
					auto findRegister = registry.registers.find(registerID);
					if(findRegister == registry.registers.end()) {
						printf("[CAN] : Error on write default request. Register (%d) not found", (uint16_t) registerID);
					}
					else {
						if(message.data_length_code == sizeof(Registry::Operation) + sizeof(Registry::Operation) + sizeof(int32_t)) {
							// If the message contains a value also, then write that value
							findRegister->second.value = value;
						}
						registry.saveDefault(registerID);
					}	
				}
			}
			return true;
		}
	}

	//----------
	void
	CANResponder::send()
	{
		for(auto & message : this->txQueue)
		{
			if(can_transmit(&message, 10 / portTICK_PERIOD_MS) != ESP_OK) {
				printf("[CAN] Transmit failed\n");
				this->errorCount++;
			}
			this->txCount++;

			// Perform a receive between each send (I think this clears the ACK queue)
			this->receive(0);
		}
		this->txQueue.clear();
	}
}
