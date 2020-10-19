#include "CANResponder.h"
#include "Registry.h"

#include "FreeRTOS.h"

#include <set>

#define CAN_PRINT_PREVIEW_ENABLED false

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
		can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_22, CAN_MODE_NORMAL);
		can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();
		can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

		//Install & Start CAN driver
		ESP_ERROR_CHECK(can_driver_install(&g_config, &t_config, &f_config));
		ESP_ERROR_CHECK(can_start());
	}

	//----------
	void
	CANResponder::update()
	{
		static auto & registry = Registry::X();
		auto & deviceID = registry.registers.at(Registry::RegisterType::DeviceID).value;

		std::set<Registry::RegisterType> readRequests;

		// handle incoming messages
		{
			can_message_t message;
			while(can_receive(&message, 0) == ESP_OK)
			{
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
					message.identifier = deviceID;
					
					auto data = message.data;
					valueAndMove<Registry::Operation>(data) = Registry::Operation::ReadResponse;
					valueAndMove<Registry::RegisterType>(data) = registerID;
					valueAndMove<int32_t>(data) = findRegister->second.value;
				}
			}
		}
	}
}