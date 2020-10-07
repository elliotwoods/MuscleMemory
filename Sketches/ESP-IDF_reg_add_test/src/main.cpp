#include "Arduino.h"
#include <driver/gpio.h>
#include "U8g2lib.h"
#include "driver/can.h"
#include "sdkconfig.h"
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

extern "C"
{
#include <u8g2_esp32_hal.h>
}

U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, 15, 4, 16);

// Set Addresses for CAN communication ---------------------------
typedef uint16_t TargetID;
TargetID board = 1;

enum Operation : uint8_t {
	Read = 0,
	Write = 1,
	WriteDefault = 2
};

enum Register : uint16_t {
	Example1 = 0,
	Example2,
	Example3
};
//-----------------------------------------------------------------



// OLED Update 
void draw(TargetID targetID, Operation operation, Register registerID, int32_t value) {
	u8g2.firstPage();
	do {
		u8g2.setFont(u8g2_font_6x12_me);

		char buf[20];
		snprintf (buf, 20, "ID: %u", targetID);
		u8g2.drawStr(10, 15, buf);

		if(operation==Operation::Write){
			u8g2.drawStr(45, 15, "write to:");
		}else if(operation==Operation::Read){
			u8g2.drawStr(45, 15, "read from:");
		}

		char buf1[20];
		snprintf (buf1, 20, "RegisterID: %u", registerID);
		u8g2.drawStr(10, 30, buf1);	
		
		char buf2[20];
		snprintf (buf2, 20, "Arg: %d", value);
		u8g2.drawStr(10, 45, buf2);
	}while (u8g2.nextPage());
}

template<typename T>
T & readAndMove(uint8_t *& data)
{
	auto & value = * (T*) data;
	data += sizeof(T);
	return value;
}

// for receiving 
void listening(void *pvParameter)
{
 for( ;; )
 {
    //Wait for message to be received
    can_message_t message;
    if (can_receive(&message, pdMS_TO_TICKS(50)) == ESP_OK) {
        printf("Message received\n");

		//Process received message
		TargetID targetID = message.identifier;
		auto dataMover = message.data;

		//Read the data out
		auto & operation = readAndMove<Operation>(dataMover);
		auto & registerID = readAndMove<Register>(dataMover);
		auto & value = readAndMove<int32_t>(dataMover);

		draw(targetID, operation, registerID, value);
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
    if (can_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        printf("Driver installed\n");
    } else {
        printf("Failed to install driver\n");
        return;
    }

    //Start CAN driver
    if (can_start() == ESP_OK) {
        printf("Driver started\n");
    } else {
        printf("Failed to start driver\n");
        return;
    }

	// create new Thread for receiving
    xTaskCreate(&listening, "RECEIVING", 2048, NULL, 5, NULL);
}



template<typename T>
void writeAndMove(uint8_t *& data, const T& value)
{
	*(T*)data = value;
	data += sizeof(T);
}

void writeReq(uint16_t targetID, Register registerID, uint32_t value){
	can_message_t message;
    message.identifier = targetID;
    message.flags = CAN_MSG_FLAG_EXTD;
    
	// Write Request data map : Total 7 Bytes
	// 	|	0	|	Operation
	// 	|  1~2 	|	Register ID
	// 	|  3~6	|	Arguments

	message.data_length_code = 7;
	auto dataMover = message.data;

	writeAndMove(dataMover, Operation::Write);
	writeAndMove(dataMover, registerID);
	writeAndMove(dataMover, value);

    //Queue message for transmission
    if (can_transmit(&message, pdMS_TO_TICKS(50)) == ESP_OK) {
        printf("Message queued for transmission\n");
		draw(targetID, Operation::Write, registerID, value);
    } else {
        printf("Failed to queue message for transmission\n");
    }
}

void readRes(){}




uint32_t _msg = 100;
void loop()
{
	if(board==1){
		writeReq(board, Register::Example1, _msg);
		delay(3000);
		_msg++;
	}
}

