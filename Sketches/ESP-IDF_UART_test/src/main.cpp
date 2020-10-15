#include "Arduino.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "soc/uart_struct.h"
 
#define ECHO_TEST_TXD  (1)
#define ECHO_TEST_RXD  (3)
#define BUF_SIZE (1024)
 
//UART part
void UART_communication(void *arg)
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
            
            //uart_write_bytes(uart_num, (const uint8_t) data, len);
        }else{
            // wrong msg format
        }
        vTaskDelay(10 / portTICK_RATE_MS);
    }
}
 
void setup()
{
    //A uart read/write example without event queue;
    xTaskCreate(UART_communication, "uart_echo_task", 2048, NULL, 10, NULL);
}

void loop(){
    vTaskDelay(500 / portTICK_RATE_MS);
}


/*\
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
    */
