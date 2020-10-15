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

#define DEVICEID	0

// Set device ID ---------------------------
typedef uint16_t DeviceID;
DeviceID device = DEVICEID;

// UART communication part ---------------------------
// Only for the router device(0) 
#if	DEVICEID == 0
	#include <stdio.h>
	#include <string.h>
	#include <stdlib.h>
	#include "esp_log.h"
	#include "nvs_flash.h"
	#include "driver/uart.h"
	#include "soc/uart_struct.h"

	#define ECHO_TEST_TXD  (1)
	#define ECHO_TEST_RXD  (3)
	#define BUF_SIZE (1024)

	//UART communication part
	void UART_communication(void *pvParameter)
	{    
		const uart_port_t uart_num = UART_NUM_0;
		uart_config_t uart_config = {
			.baud_rate = 115200,
			.data_bits = UART_DATA_8_BITS,
			.parity = UART_PARITY_DISABLE,
			.stop_bits = UART_STOP_BITS_1,
			.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,    
			.rx_flow_ctrl_thresh = 122,
		};
		//Configure UART1 parameters
		uart_param_config(uart_num, &uart_config);
	
		//Set UART1 pins(TX: IO4, RX: I05)
		uart_set_pin(uart_num, ECHO_TEST_TXD, ECHO_TEST_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
		
		//Install UART driver (we don't need an event queue here)
		//In this example we don't even use a buffer for sending data.
		uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);
	
		uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
		while(1) {
			//Read data from UART
			int len = uart_read_bytes(uart_num
				, data
				, BUF_SIZE
				, ((1000/60)/2) / portTICK_RATE_MS);
			data[len] = '\0';

			if(len==8){
				printf("msg length: %d \n", len);
				//Write data back to UART
				uint8_t _targetID = (uint8_t)data[0];
				uint8_t _operation = (uint8_t)data[1];
				uint16_t _registerID = ((uint16_t)data[3]<<8) + (uint16_t)data[2];
				uint32_t _value = ((uint32_t)data[7]<<24) 
								+ ((uint32_t)data[6]<<16)
								+ ((uint32_t)data[5]<<8)
								+ (uint32_t)data[4];
				printf("%u,%u,%u,%u\n",_targetID,_operation,_registerID,_value);
				//printf("%u,%u,%u,%u,%u,%u,%u,%u\n",(uint8_t)data[0],(uint8_t)data[1],(uint8_t)data[2],(uint8_t)data[3],(uint8_t)data[4],(uint8_t)data[5],(uint8_t)data[6],(uint8_t)data[7]);
				//uart_write_bytes(uart_num, (const uint8_t) data, len);
			}else{
				// wrong msg format
			}
			vTaskDelay(10 / portTICK_RATE_MS);
		}
	}	
#endif

extern "C"
{
#include <u8g2_esp32_hal.h>
}

// Rotary Encoder part
Dial dial;

//-----------------------------------------------------------------

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

void transmitting(uint16_t targetID
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
			auto &operation = readAndMove<Registry::Operation>(dataMover);
			auto &registerID = readAndMove<Registry::RegisterType>(dataMover);
			auto &value = readAndMove<int32_t>(dataMover);
			printf(" op: %d \n", operation);
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
	}
}
// CAN communication part END-----------------------------------------------------

void setup()
{
	#if	DEVICEID != 0
		Serial.begin(115200);
	#endif

	// set Rotary encoder
	dial.init(gpio_num_t::GPIO_NUM_34, gpio_num_t::GPIO_NUM_35);
	initDial();

	// set DeviceID if needed
	if(device!=0){
		auto & registry = Registry::X();
		registry.registers[Registry::RegisterType::deviceID].value = device;
	}
	
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

	// start UART receiving Task
	#if	DEVICEID == 0
		xTaskCreate(UART_communication, "uart_echo_task", 2048, NULL, 10, NULL); 
	#endif

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
