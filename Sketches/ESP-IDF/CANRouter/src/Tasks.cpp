#include "Tasks.h"

#include <stdio.h>
#include <string.h>
#include <vector>

#include "Arduino.h"

extern "C"
{
	#include "driver/uart.h"
	#include "driver/can.h"
	#include "crypto/base64.h"
}

#include "FreeRTOS.h"


uart_port_t port = uart_port_t::UART_NUM_0;
#ifdef ARDUINO
	#define TX_PIN GPIO_NUM_1
	#define RX_PIN GPIO_NUM_2
#else
	#define TX_PIN 1
	#define RX_PIN 2
#endif

#define CAN_TX_GPIO_NUM GPIO_NUM_21
#define CAN_RX_GPIO_NUM GPIO_NUM_22

QueueHandle_t canToSerial;
QueueHandle_t serialToCan;

size_t canToSerialCount = 0;
size_t serialToCanCount = 0;

char serialTerminator[1];

//-----------
void
canInit()
{
	can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(CAN_TX_GPIO_NUM, CAN_RX_GPIO_NUM, CAN_MODE_NORMAL);
	{
		g_config.rx_queue_len = 128;
		g_config.tx_queue_len = 128;
	}

	can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();

	can_filter_config_t f_config;
	{
		f_config.acceptance_code = 0;
		f_config.acceptance_mask = 0xFFFFFFFF; // Accept 1023 and our actual device ID
		f_config.single_filter = true;
	}

	//Install & Start CAN driver
	ESP_ERROR_CHECK(can_driver_install(&g_config, &t_config, &f_config));
	ESP_ERROR_CHECK(can_start());

	//Setup bus alerts
	ESP_ERROR_CHECK(can_reconfigure_alerts(CAN_ALERT_ABOVE_ERR_WARN | CAN_ALERT_ERR_PASS | CAN_ALERT_BUS_OFF, NULL));

	canToSerial = xQueueCreate(128, sizeof(can_message_t));
	serialToCan = xQueueCreate(128, sizeof(can_message_t));
}

//-----------
void
canUpdate()
{
	// Receive
	can_message_t message;
	while(can_receive(&message, 1 / portTICK_PERIOD_MS) == ESP_OK) {
		xQueueSend(canToSerial, &message, 1 / portTICK_PERIOD_MS);
	}

	// Transmit
	while(xQueueReceive(serialToCan, &message, 1 / portTICK_PERIOD_MS) == pdTRUE) {
		serialToCanCount++;
		can_transmit(&message, 1 / portTICK_PERIOD_MS);
	}
}

//-----------
void
canTask(void *)
{
	while(true) {
		canUpdate();
		vTaskDelay(1);
	}
}

//----------
void
serialInit()
{
	serialTerminator[0] = 255;

	/*
	uart_config_t uart_config = {0};
	{
		uart_config.baud_rate = 956000;
		uart_config.data_bits = UART_DATA_8_BITS;
		uart_config.parity    = UART_PARITY_DISABLE;
		uart_config.stop_bits = UART_STOP_BITS_1;
		uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
	}	

	{
		auto result = uart_param_config(port, &uart_config);
		ESP_ERROR_CHECK(result);
	}

	{
		auto result = uart_set_pin(port
			, TX_PIN
			, RX_PIN
			, UART_PIN_NO_CHANGE
			, UART_PIN_NO_CHANGE);
		ESP_ERROR_CHECK(result);
	}

	{
		auto result = uart_driver_install(port
			, 2048
			, 2048
			, 0
			, NULL
			, 0);
		ESP_ERROR_CHECK(result);
	}
	

	// write a term to start with (so the next message)
	uart_write_bytes(port, serialTerminator, 1);
*/

	Serial.begin(256000);
	Serial.setTimeout(1);
}


std::vector<uint8_t> serialIncoming;

//----------
void decodeSerialBuffer()
{
	size_t decodedLength;
	auto decoded = base64_decode((const unsigned char *) serialIncoming.data()
		, serialIncoming.size()
		, &decodedLength);
	if(decoded) {
		if(decodedLength >= 17) {
			can_message_t message;
			memcpy(&message, decoded, 17);
			xQueueSend(serialToCan
				, &message
				, 1 / portTICK_PERIOD_MS);
		}
		else {
			printf("Invalid message from serial. Length (%d) < (%d)\n", decodedLength, sizeof(can_message_t));
		}
		free(decoded);
	}
}

//----------
void serialUpdate()
{
	// Transmit to network
	{
		/*
		uint8_t buffer[1024];
		int rxLength = uart_read_bytes(port
			, buffer
			, 1024
			, 20 / portTICK_RATE_MS);

		for(int i=0; i<rxLength; i++) {
			serialToCanCount++;
			auto value = buffer[i];
			if(value == 0xFF) {
				decodeSerialBuffer();
			}
			else {
				serialIncoming.push_back(value);
			}
		}*/

		while(Serial.available()) {
			uint8_t buffer[1024];
			auto length = Serial.readBytes(buffer, 1024);

			for(int i=0; i<length; i++) {
				auto value = buffer[i];
				if(value == 0xFF) {
					decodeSerialBuffer();
				}
				else {
					serialIncoming.push_back(value);
				}
			}
		}
	}

	// Receive from network
	can_message_t message;
	while(xQueueReceive(canToSerial, &message, 1 / portTICK_PERIOD_MS) == pdTRUE) {
		canToSerialCount++;

		size_t encodedLength;
		auto encodedString = base64_encode((const unsigned char *) &message
			, sizeof(can_message_t)
			, &encodedLength);


		if(encodedString == NULL) {
			printf("Data is NULL\n");
		}

		Serial.write((const uint8_t*) encodedString, encodedLength);
		Serial.write(0xFF);

		/*
		// Write CAN message
		uart_write_bytes(port
			, (const char*) encodedString
			, encodedLength);
		
		// Write terminator
		uart_write_bytes(port
			, serialTerminator
			, 1);
		*/

		// We need to deallocate the string
		free(encodedString);
	}
}

//----------
void serialTask(void *)
{
	while(true) {
		serialUpdate();
		vTaskDelay(1);
	}
}


size_t
getCanToSerialCount()
{
	auto value = canToSerialCount;
	canToSerialCount = 0;
	return value;
}


size_t
getSerialToCanCount()
{
	auto value = serialToCanCount;
	serialToCanCount = 0;
	return value;
}