#include "CANResponder.h"
#include "Registry.h"

#ifdef ARDUINO
#include "FreeRTOS.h"
#else
#include <freertos/FreeRTOS.h>
#endif

#include "esp_log.h"

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
	CANResponder::CANResponder(Control::FilteredTarget & filteredTarget)
	: filteredTarget(filteredTarget)
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
		}

		can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();

		can_filter_config_t f_config;
		{
			this->deviceIDMask = (uint16_t) getRegisterValue(Registry::RegisterType::DeviceID);
			f_config.acceptance_code = this->deviceIDMask << 3;
			f_config.acceptance_mask = 0xFFFFFFFF - ((1024 - 1) << 3);
			f_config.single_filter = true;
		}

		//Install & Start CAN driver
		ESP_ERROR_CHECK(can_driver_install(&g_config, &t_config, &f_config));
		ESP_ERROR_CHECK(can_start());

		//Setup bus alerts
		ESP_ERROR_CHECK(can_reconfigure_alerts(CAN_ALERT_ABOVE_ERR_WARN | CAN_ALERT_ERR_PASS | CAN_ALERT_BUS_OFF, NULL));
	}

	//----------
	void
	CANResponder::deinit()
	{
		ESP_ERROR_CHECK(can_stop());
		
		// Clear the Rx queue
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
		if (CAN_PRINT_PREVIEW_ENABLED) {
			if(this->rxCount > 0 || this->txCount > 0 || this->errorCount > 0) {
				printf("[CAN] Rx : (%u), Tx : (%u), Errors : (%u)\n", this->rxCount, this->txCount, this->errorCount);
			}
		}

		// Check if Device ID changed
		{
			auto deviceIDMask = (uint16_t) getRegisterValue(Registry::RegisterType::DeviceID);
			if(deviceIDMask != this->deviceIDMask) {
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
	void
	CANResponder::updateTask()
	{
		// For reads and writes we use more complex manner than simple get/set functions
		static auto & registry = Registry::X();

		std::set<Registry::RegisterType> readRequests;

		// handle incoming messages
		{
			can_message_t message;
			bool firstRead = true;

			// Wait up to 100 ms on first read
			while(can_receive(&message, firstRead ? 100 / portTICK_PERIOD_MS : 0) == ESP_OK)
			{
				firstRead = false;
				this->rxCount++;

				auto dataMover = message.data;
				auto operation = valueAndMove<Registry::Operation>(dataMover);
				auto registerID = valueAndMove<Registry::RegisterType>(dataMover);
				auto value = valueAndMove<int32_t>(dataMover);

				// Print message preview
				if (CAN_PRINT_PREVIEW_ENABLED) {
					printf("Can Rx. ID [%d], Flags [%d], Data [%X %X %X %X %X %X %X %X]\n"
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
					printf("RETR\n");
					for(const auto & defaultRegisterID : Registry::defaultRegisterReads) {
						readRequests.insert(defaultRegisterID);
					}
				}
				else if(operation == Registry::Operation::ReadRequest) {
					// Queue individual read response
					readRequests.insert(registerID);
				}
				else if(operation == Registry::Operation::WriteRequest) {
					// Perform write requests
					auto findRegister = registry.registers.find(registerID);
					if(findRegister == registry.registers.end()) {
						printf("[CAN] : Error on write request. Register (%d) not found", (uint16_t) registerID);
					}
					else {
						findRegister->second.value = value;
						if(registerID == Registry::RegisterType::TargetPosition) {
							this->filteredTarget.notifyTargetChange();
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
		}

		// respond to any read requests
		{
			for(const auto & registerID : readRequests) {
				auto findRegister = registry.registers.find(registerID);
				if(findRegister == registry.registers.end()) {
					printf("[CAN] : Error on read request. Register (%d) not found", (uint16_t) registerID);
				}
				else {
					can_message_t message;
					message.flags = CAN_MSG_FLAG_EXTD;
					
					auto data = message.data;
					valueAndMove<Registry::Operation>(data) = Registry::Operation::ReadResponse;
					valueAndMove<Registry::RegisterType>(data) = registerID;
					valueAndMove<int32_t>(data) = findRegister->second.value;

					if(can_transmit(&message, 1 / portTICK_PERIOD_MS) != ESP_OK) {
						printf("[CAN] Transmit failed\n");
						this->errorCount++;
					}
					this->txCount++;
				}
			}
		}

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
}
