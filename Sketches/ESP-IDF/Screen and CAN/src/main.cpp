#include "Arduino.h"
#include <driver/gpio.h>
#include "U8g2lib.h"
#include "driver/can.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "GuiController.h"

#include "Registry.h"
#include "Panels/RegisterList.h"
#include "Dial.h"

#define DEVICEID	1

// Set device ID ---------------------------
typedef uint8_t DeviceID;
DeviceID device = DEVICEID;

extern "C"
{
#include <u8g2_esp32_hal.h>
}

// Rotary Encoder part
Dial dial;

// OLED Part -----------------------------------------------------
U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, 15, 4, 16);

uint8_t registerySize = Registry::X().registers.size();
uint8_t maxRows = 4;
uint8_t totalPages = registerySize / maxRows + (registerySize % maxRows>0);
uint8_t selectPage = 0;
uint8_t depth = 0;

// CAN communication part -----------------------------------------------------
// Transmit Part
template <typename T>
void writeAndMove(uint8_t *&data, const T &value)
{
	*(T *)data = value;
	data += sizeof(T);
}

void transmitting(uint8_t targetID
	, Registry::Operation operation
	, Registry::RegisterType registerID
	, uint32_t value)
{
	can_message_t message;
	message.identifier = device;
	message.flags = CAN_MSG_FLAG_EXTD;

	// Write Request data map : Total 8 Bytes
	// 	|	0	|	Target ID
	// 	|	1	|	Operation
	// 	|  2,3 	|	Register ID
	// 	|4,5,6,7|	Value

	message.data_length_code = 8;
	auto dataMover = message.data;

	writeAndMove(dataMover, targetID);
	writeAndMove(dataMover, operation);
	writeAndMove(dataMover, registerID);
	writeAndMove(dataMover, value);

	//Queue message for transmission
	if (can_transmit(&message, pdMS_TO_TICKS(50)) == ESP_OK)
	{
		#if	DEVICEID != 0
			printf("Message queued for transmission\n");
		#endif
		//draw(targetID, Operation::WriteRequest, registerID, value);
	}
	else
	{
		printf("Failed to queue message for transmission\n");
	}
}

// Receive Part
template <typename T>
T &readAndMove(uint8_t *&data)
{
	auto &value = *(T *)data;
	data += sizeof(T);
	return value;
}

// for receiving
void receiving(void *pvParameter)
{
	auto & registry = Registry::X();
	for (;;)
	{
		//Wait for message to be received
		can_message_t message;
		if (can_receive(&message, pdMS_TO_TICKS(50)) == ESP_OK)
		{
			printf("Message received\n");
			//Process received message
			DeviceID senderID = message.identifier;
			auto dataMover = message.data;
			//printf("Message received %d\n",senderID);
			//Read the data out
			auto &targetID = readAndMove<int8_t>(dataMover);
			//--------------------------------------------------------------- add filter step later?
			
			if(targetID != device){
				break;
			}	
			
			auto &operation = readAndMove<Registry::Operation>(dataMover);
			auto &registerID = readAndMove<Registry::RegisterType>(dataMover);
			auto &value = readAndMove<int32_t>(dataMover);
			//printf(" op: %d \n", operation);
			//Identify the operation

			switch (operation)
			{
			case Registry::Operation::WriteRequest:
				printf(" * WriteRequest from %d ,", senderID);
				printf(" * regID %d value %d \n", registerID, value);
				registry.registers[registerID].value = value;
				break;
			case Registry::Operation::ReadRequest:
				printf(" * ReadRequest from %d ,", senderID);
				printf(" * regID %d \n", registerID);
				value = registry.registers[registerID].value;
				transmitting(senderID, Registry::Operation::ReadResponse, registerID, value);
				break;
			case Registry::Operation::ReadResponse:
				printf(" * ReadResponse from %d,", senderID);
				printf(" * regID %d value %d \n", registerID, value);
 				break;
			default:
				break;
			}

			//draw(senderID, operation, registerID, value);
		}
		vTaskDelay(10 / portTICK_RATE_MS);
	}
}
// CAN communication part END-----------------------------------------------------



void setup()
{
	Serial.begin(115200);

	// set Rotary encoder
	dial.init(gpio_num_t::GPIO_NUM_34, gpio_num_t::GPIO_NUM_35);
	initDial();

	// set DeviceID if needed
	auto & registry = Registry::X();
	registry.registers[Registry::RegisterType::deviceID].value = device;
	
	// set OLED ---------------------------------------------------------
	u8g2.begin();
	GuiController::X().init(u8g2, std::make_shared<Panels::RegisterList>());

	// set CAN ----------------------------------------------------------
	//Initialize configuration structures using macro initializers
	// NOTE : we define CAN_MODE_UNUSED to be GPIO_NUM_MAX in can.h
	can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_22, CAN_MODE_NORMAL);
	can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();
	can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

	//Install CAN driver
	if (can_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
	{
		printf("Driver installed\n");
	}
	else
	{
		printf("Failed to install driver\n");
		return;
	}

	//Start CAN driver
	if (can_start() == ESP_OK)
	{
		printf("Driver started\n");
	}
	else
	{
		printf("Failed to start driver\n");
		return;
	}

	// start CAN receiving Task
	xTaskCreate(&receiving, "RECEIVING", 2048, NULL, 10, NULL);	
}

uint32_t _msg = 100;
bool trg = true;
void loop()
{
	GuiController::X().update();
	delay(10);	
}