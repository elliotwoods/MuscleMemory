#include "Arduino.h"
#include <driver/gpio.h>
#include "U8g2lib.h"
#include "driver/can.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "Registry.h"

extern "C"
{
#include <u8g2_esp32_hal.h>
}

U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, 15, 4, 16);

// Set Addresses for CAN communication ---------------------------
typedef uint16_t DeviceID;
DeviceID device = 1;

enum Operation : uint8_t
{
	WriteRequest = 0,
	ReadRequest = 1,
	ReadResponse = 2
};

//-----------------------------------------------------------------

// OLED Update
void draw()
{
	auto & registry = Registry::X();

	u8g2.firstPage();
	char message[100];

	do
	{
		u8g2.setFont(u8g2_font_profont10_mr);

		int y = 1;

		{
			sprintf(message, "Device ID: %d", device);
			u8g2.drawStr(5, y++ * 10, message);
		}

		for (const auto &it : registry.registers)
		{
			sprintf(message, "%s : %d", it.second.name.c_str(), it.second.value);
			u8g2.drawStr(5, y++ * 10, message);
		}

	} while (u8g2.nextPage());
}

// Transmit Part
template <typename T>
void writeAndMove(uint8_t *&data, const T &value)
{
	*(T *)data = value;
	data += sizeof(T);
}

void transmitting(uint16_t targetID
	, Operation operation
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
	// 	|4,5,6,7|	Arguments

	message.data_length_code = 8;
	auto dataMover = message.data;

	writeAndMove(dataMover, targetID);
	writeAndMove(dataMover, operation);
	writeAndMove(dataMover, registerID);
	writeAndMove(dataMover, value);

	//Queue message for transmission
	if (can_transmit(&message, pdMS_TO_TICKS(50)) == ESP_OK)
	{
		printf("Message queued for transmission\n");
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

			//Read the data out
			auto &targetID = readAndMove<int16_t>(dataMover);
			//--------------------------------------------------------------- add filter step later?
			auto &operation = readAndMove<Operation>(dataMover);
			auto &registerID = readAndMove<Registry::RegisterType>(dataMover);
			auto &value = readAndMove<int32_t>(dataMover);
			printf(" op: %d \n", operation);
			//Identify the operation

			switch (operation)
			{
			case Operation::WriteRequest:
				printf(" * WriteRequest from %d ,", senderID);
				printf(" * regID %d value %d \n", registerID, value);
				registry.registers[registerID].value = value;
				break;
			case Operation::ReadRequest:
				printf(" * ReadRequest from %d ,", senderID);
				printf(" * regID %d \n", registerID);
				value = registry.registers[registerID].value;
				transmitting(senderID, Operation::ReadResponse, registerID, value);
				break;
			case Operation::ReadResponse:
				printf(" * ReadResponse from %d,", senderID);
				printf(" * regID %d value %d \n", registerID, value);
				break;
			default:
				break;
			}

			//draw(senderID, operation, registerID, value);
		}
	}
}

void setup()
{
	Serial.begin(115200);

	// set OLED ---------------------------------------------------------
	u8g2.begin();

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

	// create new Thread for receiving
	xTaskCreate(&receiving, "RECEIVING", 2048, NULL, 5, NULL);
}

uint32_t _msg = 100;
bool trg = true;
void loop()
{
	draw();
	delay(500);

	if (device == 1 && trg)
	{
		trg = false;
		delay(1000);
		transmitting(2, Operation::WriteRequest, Registry::RegisterType::CurrentPosition, 33);
		delay(500);
		transmitting(2, Operation::WriteRequest, Registry::RegisterType::CurrentVelocity, 33);
		delay(500);
		transmitting(2, Operation::WriteRequest, Registry::RegisterType::TargetPosition, 33);
		delay(500);
		transmitting(2, Operation::ReadRequest, Registry::RegisterType::CurrentVelocity, 0);
		delay(5000);
	}
}
