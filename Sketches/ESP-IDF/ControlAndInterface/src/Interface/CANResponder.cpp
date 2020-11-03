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
	void
	CANResponder::init()
	{
		// set CAN ----------------------------------------------------------
		//Initialize configuration structures using macro initializers
		// NOTE : this will fail to compile
		// We change line 108 of can.h to define CAN_IO_UNUSED to be GPIO_NUM_MAX
		can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, CAN_MODE_NORMAL);
		can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();
		can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

		//Install & Start CAN driver
		ESP_ERROR_CHECK(can_driver_install(&g_config, &t_config, &f_config));
		ESP_ERROR_CHECK(can_start());

		//Setup bus alerts
		can_reconfigure_alerts(CAN_ALERT_ABOVE_ERR_WARN | CAN_ALERT_ERR_PASS | CAN_ALERT_BUS_OFF, NULL);
	}

	//----------
	void
	CANResponder::update()
	{
		static auto & registry = Registry::X();
		auto & ourDeviceID = registry.registers.at(Registry::RegisterType::DeviceID).value;

		std::set<Registry::RegisterType> readRequests;

		uint16_t rxCount = 0;
		uint16_t txCount = 0;
		uint16_t errorCount = 0;

		// handle incoming messages
		{
			can_message_t message;
			while(can_receive(&message, 0) == ESP_OK)
			{
				auto messageDeviceID = message.identifier;
				if(messageDeviceID != ourDeviceID) {
					continue;
				}
				rxCount++;

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
						errorCount++;
					}
					txCount++;
				}
			}
		}

		// Read alerts
		{
			uint32_t alerts;
			while(can_read_alerts(&alerts, 0) == ESP_OK) {
				if (alerts & CAN_ALERT_ABOVE_ERR_WARN) {
					ESP_LOGI(MOD_TAG, "Surpassed Error Warning Limit");
					errorCount++;
				}
				if (alerts & CAN_ALERT_ERR_PASS) {
					ESP_LOGI(MOD_TAG, "Entered Error Passive state");
					errorCount++;
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
					errorCount++;
				}
				if (alerts & CAN_ALERT_BUS_RECOVERED) {
					//Bus recovery was successful, exit control task to uninstall driver
					ESP_LOGI(MOD_TAG, "Bus Recovered");
					break;
				}
			}
		}

		// Save info to registry
		registry.registers.at(Registry::RegisterType::CANRxThisFrame).value = (int32_t) rxCount;
		registry.registers.at(Registry::RegisterType::CANTxThisFrame).value = (int32_t) txCount;
		registry.registers.at(Registry::RegisterType::CANErrorsThisFrame).value = (int32_t) errorCount;
		if(rxCount > 0 || txCount > 0 || errorCount > 0) {
			printf("[CAN] Rx : (%u), Tx : (%u), Errors : (%u)\n", rxCount, txCount, errorCount);
		}
	}
}
