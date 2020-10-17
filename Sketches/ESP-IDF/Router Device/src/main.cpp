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
#define TICK_TIME	0

// for avoiding WDT bug
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

// Set device ID ---------------------------
typedef uint8_t DeviceID;
DeviceID device = DEVICEID;

#include "Devices/I2C.h"
#include "GUI/U8G2HAL.h"


// UART communication part -----------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"

extern "C" {
	#include "crypto/base64.h"
}

#define ECHO_TEST_TXD  (1)
#define ECHO_TEST_RXD  (3)
#define BUF_SIZE (1024)
const uart_port_t uart_num = UART_NUM_0;
uint8_t* data;

// Rotary Encoder part
Dial dial;

// OLED Part -----------------------------------------------------
//U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, 15, 4, 16);

U8G2 u8g2;

#define msgLength 5
char msg[msgLength][100];

void draw(){
	u8g2.setFont(u8g2_font_nerhoe_tr);
	for(int i=0;i<msgLength;i++){
		u8g2.drawStr(5,8+12*i,msg[i]);
	}			
	
}

void screenUpdate(char newMsg[100]){

	for(int i=msgLength-1;i>0;i--){
		strcpy(msg[i],msg[i-1]);
	}
	strcpy(msg[0],newMsg);
	
	u8g2.firstPage();
	do {
		draw();
	} while (u8g2.nextPage());
	//vTaskDelay(10 / portTICK_RATE_MS);
}


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
	if (can_transmit(&message, pdMS_TO_TICKS(TICK_TIME)) == ESP_OK)
	{
		printf("succesed to queue message for transmission\n");
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
		// for avoiding WDT bug
		TIMERG0.wdt_wprotect=TIMG_WDT_WKEY_VALUE;
		TIMERG0.wdt_feed=1;
		TIMERG0.wdt_wprotect=0;

		//Wait for message to be received
		can_message_t message;
		if (can_receive(&message, pdMS_TO_TICKS(TICK_TIME)) == ESP_OK)
		{
			//printf("Message received\n");

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
			if(operation != Registry::Operation::ReadResponse){
				break;
			}
			auto &registerID = readAndMove<Registry::RegisterType>(dataMover);
			auto &value = readAndMove<int32_t>(dataMover);


			// on OLED
			char _msg[100];
			sprintf(_msg,"<< ID %d,Reg %d :%d\n",senderID,registerID,value);
			screenUpdate(_msg);

			//Identify the operation
			uint8_t data[8];
			data[0] = (uint8_t)senderID;
			data[1] = (uint8_t)operation;
			data[3] = (uint8_t)(registerID>>8);
			data[2] = (uint8_t)(registerID);
			data[7] = (uint8_t)(value>>24);
			data[6] = (uint8_t)(value>>16);
			data[5] = (uint8_t)(value>>8);
			data[4] = (uint8_t)(value);

			size_t encodedLength;
			auto encodedData = base64_encode(data, 8, &encodedLength);
			auto encodedLengthShort = (uint8_t) encodedLength;

			// Send the data to the PC over UART 
			uint8_t delimiter = 0;
			//uart_write_bytes(uart_num, (const char *) &encodedLengthShort, 1);
			printf("stof \n");
			uart_write_bytes(uart_num, (const char *) encodedData, encodedLength);
			uart_write_bytes(uart_num, (const char *) &delimiter, 1);
			printf("endof \n");
			free(encodedData);
		}
		//vTaskDelay(10 / portTICK_RATE_MS);
		//printf("-------et--- \n");
	}
}
// CAN communication part END-----------------------------------------------------



void setup()
{
	//Serial.begin(115200);
	// set Rotary encoder
	dial.init(gpio_num_t::GPIO_NUM_34, gpio_num_t::GPIO_NUM_35);
	initDial();
	
	// set OLED ---------------------------------------------------------
	auto & i2c = Devices::I2C::X();
	i2c.init();

	u8g2_Setup_ssd1306_i2c_128x64_noname_1(u8g2.getU8g2()
		, U8G2_R0
		, u8g2_byte_hw_i2c_esp32
		, u8g2_gpio_and_delay_esp32);
	u8x8_SetPin(u8g2.getU8x8(), U8X8_PIN_RESET, GPIO_NUM_16);
	u8x8_SetI2CAddress(u8g2.getU8x8(), 0x3c);
	u8g2.begin();

	// test - delete this later
	u8g2.firstPage();
	do {
		u8g2.drawCircle(32, 32, 32);
	} while(u8g2.nextPage());

	// set CAN ----------------------------------------------------------
	//Initialize configuration structures using macro initializers
	// NOTE : we define CAN_MODE_UNUSED to be GPIO_NUM_MAX in can.h
	can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_22, CAN_MODE_NORMAL);
	can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();
	can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

	//Install & Start CAN driver
	can_driver_install(&g_config, &t_config, &f_config);
	can_start();
	// Setup UART
	uart_config_t uart_config = {
		.baud_rate = 115200,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,    
		.rx_flow_ctrl_thresh = 122,
	};
	//Configure UART parameters
	uart_param_config(uart_num, &uart_config);

	//Set UART1 pins
	uart_set_pin(uart_num, ECHO_TEST_TXD, ECHO_TEST_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	
	//Install UART driver (we don't need an event queue here)
	//In this example we don't even use a buffer for sending data.
	uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0);
	data = (uint8_t*) malloc(BUF_SIZE);
	// start CAN receiving Task
	xTaskCreate(&receiving, "RECEIVING", 2048, NULL, 10, NULL);	


}






unsigned long T = millis();

void loop()
{
	printf("---l--- \n");
	int incomingByte = 0; 

	//Read data from UART
	int len = uart_read_bytes(uart_num
		, data
		, BUF_SIZE
		, ((1000/60)/2) / portTICK_RATE_MS);
	data[len] = '\0';

	if(incomingByte>=8){
		uint8_t _targetID = (uint8_t)data[0];
		Registry::Operation _operation = (Registry::Operation)data[1];
		Registry::RegisterType _registerID = Registry::RegisterType(((uint16_t)data[3]<<8) + (uint16_t)data[2]);
		uint32_t _value = ((uint32_t)data[7]<<24) 
						+ ((uint32_t)data[6]<<16)
						+ ((uint32_t)data[5]<<8)
						+ (uint32_t)data[4];
		
		transmitting(_targetID,_operation,_registerID,_value);

		// on OLED
		char _msg[100];
		sprintf(_msg,">>ID %d,Opr %d,Reg %d :%d\n",_targetID,_operation,_registerID,_value);
		screenUpdate(_msg);
		
	}
	if(millis()-T>5000){

		Registry::Operation op = Registry::Operation::WriteRequest;
		Registry::RegisterType rt = Registry::RegisterType::CurrentPosition;

		transmitting(1,op,rt,millis()%1000);
		op = Registry::Operation::ReadRequest;
		rt = Registry::RegisterType::CurrentPosition;

		transmitting(1,op,rt,421);
		T = millis();
	}
}