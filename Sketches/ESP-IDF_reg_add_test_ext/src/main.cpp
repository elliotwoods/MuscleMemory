#include "Arduino.h"
#include <driver/gpio.h>
#include "U8g2lib.h"
#include "driver/can.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include <string>
#include <map>

//#include "registry.h"
class Registry {
public:
	enum RegisterType : uint16_t {
		CurrentPosition = 0,
		CurrentVelocity = 1,
		TargetPosition = 2,
		TargetVelocity = 3,

		CurrentI = 10,
		MaximumI = 11,
		CurrentVBus = 12
	};

	struct Register {
		std::string name;
		int32_t data;
	};

	std::map<RegisterType, Register> registers {
		{ RegisterType::CurrentPosition, {
			"CurrentPosition"
			, 30
		}},
		{ RegisterType::CurrentVelocity, {
			"CurrentVelocity"
			, 0
		}},
		{ RegisterType::TargetPosition, {
			"TargetPosition"
			, 0
		}},
		{ RegisterType::TargetVelocity, {
			"TargetVelocity"
			, 0
		}},
		{ RegisterType::CurrentI, {
			"CurrentI"
			, 0
		}},
		{ RegisterType::MaximumI, {
			"MaximumI"
			, 0
		}},
		{ RegisterType::CurrentVBus, {
			"CurrentVBus"
			, 0
		}}
	};
};

extern "C"
{
#include <u8g2_esp32_hal.h>
}

U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, 15, 4, 16);

// Set Addresses for CAN communication ---------------------------
Registry reg;

typedef uint16_t BoardID;
BoardID board = 2;

enum Operation : uint8_t {
	WriteRequest = 0,
	ReadRequest = 1,
	ReadResponse = 2
};

//-----------------------------------------------------------------



// OLED Update 
void draw() {
	u8g2.firstPage();
	do {
		u8g2.setFont(u8g2_font_profont10_mr);
		char buf[20];
		snprintf (buf, 20, "Board ID: %d", board);
		u8g2.drawStr(5, 8, buf);
		
		std::map<Registry::RegisterType, Registry::Register>::iterator iter;
		for(iter = reg.registers.begin(); iter != reg.registers.end(); iter++){
			buf[0] ='\0';
			snprintf (buf, 20, "Board ID: %d", iter.name);
			//reg.registers[iter].data
		}
		
		
		/*
		if(operation==Operation::WriteRequest){
			u8g2.drawStr(45, 15, "write to:");
		}else if(operation==Operation::ReadRequest){
			u8g2.drawStr(45, 15, "read from:");
		}

		char buf1[20];
		snprintf (buf1, 20, "RegisterID: %u", registerID);
		u8g2.drawStr(10, 30, buf1);	
		
		char buf2[20];
		snprintf (buf2, 20, "Arg: %d", value);
		u8g2.drawStr(10, 45, buf2);
		*/
	}while (u8g2.nextPage());
}







// Transmit Part
template<typename T>
void writeAndMove(uint8_t *& data, const T& value)
{
	*(T*)data = value;
	data += sizeof(T);
}

void transmitting(uint16_t targetID, Operation operation, Registry::RegisterType registerID, uint32_t value){
	can_message_t message;
    message.identifier = board;
    message.flags = CAN_MSG_FLAG_EXTD;
    
	// Write Request data map : Total 8 Bytes
	// 	|	0	|	Target ID
	// 	|	1	|	Operation
	// 	|  2,3 	|	Register ID
	// 	|4,5,6,7|	Arguments

	message.data_length_code = 8;
	auto dataMover = message.data;

	writeAndMove(dataMover, targetID);
	writeAndMove(dataMover, Operation::WriteRequest);
	writeAndMove(dataMover, registerID);
	writeAndMove(dataMover, value);

    //Queue message for transmission
    if (can_transmit(&message, pdMS_TO_TICKS(50)) == ESP_OK) {
        printf("Message queued for transmission\n");
		//draw(targetID, Operation::WriteRequest, registerID, value);
    } else {
        printf("Failed to queue message for transmission\n");
    }
}


// Receive Part
template<typename T>
T & readAndMove(uint8_t *& data)
{
	auto & value = * (T*) data;
	data += sizeof(T);
	return value;
}

// for receiving 
void receiving(void *pvParameter)
{
 for( ;; )
 {
    //Wait for message to be received
    can_message_t message;
    if (can_receive(&message, pdMS_TO_TICKS(50)) == ESP_OK) {
        printf("Message received\n");

		//Process received message
		BoardID senderID = message.identifier;
		auto dataMover = message.data;

		//Read the data out
		auto & targetID = readAndMove<int16_t>(dataMover);
		//--------------------------------------------------------------- add filter step later?		
		auto & operation = readAndMove<Operation>(dataMover);
		auto & registerID = readAndMove<Registry::RegisterType>(dataMover);
		auto & value = readAndMove<int32_t>(dataMover);

		//Identify the operation

		switch(operation){
			case Operation::WriteRequest:
				printf(" * WriteRequest from %d \n",senderID);
				printf(" * regID %d value %d \n", registerID, value);
				reg.registers[registerID].data = value;
				break;
			case Operation::ReadRequest:
				printf(" * ReadRequest from %d \n",senderID);
				printf(" * regID %d value %d \n", registerID, value);
				value = reg.registers[registerID].data;
				transmitting(senderID,Operation::ReadResponse,registerID,value);
				break;
			case Operation::ReadResponse:
				printf(" * ReadResponse from %d \n",senderID);
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
    xTaskCreate(&receiving, "RECEIVING", 2048, NULL, 5, NULL);



}


uint32_t _msg = 100;
void loop()
{
	draw();
	delay(1000);
	/*
	if(board==1){
		transmitting(board, Registry::RegisterType::CurrentI, _msg);
		delay(3000);
		_msg++;
	}
	*/
}




