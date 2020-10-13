#include <Arduino.h>
#include "sine_stepper.h"
#include "AS5047.h"
#include "MotorDriver.h"
#include "I2C.h"
#include "INA219.h"
#include "EncoderCalibration.h"
#include "DriveController.h"
#include "U8g2lib.h"
#include "U8G2HAL.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"


// for Micro Seconds 
// for using: ets_delay_us(value)
// ** It is mentioned as Prototype library ** 
#include <rom/ets_sys.h>	

//#define ENABLE_OLED

#ifdef ENABLE_OLED
U8G2 oled;
#endif

MotorDriver motorDriver;
AS5047 as5047;
EncoderCalibration encoderCalibration;
DriveController driveController(motorDriver, as5047, encoderCalibration);

INA219 ina219;

#ifdef ENABLE_OLED
void draw() {

	as5047.drawDebug(oled);
}
#endif

void sendTorque(void *pvParameter){
	while(1){
		int8_t TQ[5] = {8,16,24,32,64};
		for(int j = 0; j<5; j++){


			uint16_t count = 0;
			int8_t tq = TQ[j];
			int64_t lastPosition = encoderCalibration.currentPosition(as5047);
			int64_t totalPosition = 0;
			auto start = micros();
			while(micros() < start + 1e6) {
				driveController.applyTorque(tq, false);
				int64_t nowPosition = encoderCalibration.currentPosition(as5047);
				if(lastPosition> 1<<13 &&  nowPosition< 1<<8){
					totalPosition += 1<<14;
				}
				if(nowPosition> 1<<13 &&  lastPosition< 1<<8){
					totalPosition -= 1<<14;
				}
				totalPosition += nowPosition-lastPosition;
				//printf("last %lld, now %lld, = %lld\n", lastPosition, nowPosition, nowPosition-lastPosition);
				lastPosition = nowPosition;
				
				count++;
			}
			
			printf("----------------------Touque: %d. Velocity  %lld (/s). %d ticks \n", tq, totalPosition, count);
		#ifdef ENABLE_OLED
			oled.firstPage();
			do {
				draw();
			} while (oled.nextPage());
		#endif
			delay(1000);
		};
		delay(5000);
	}
}


void setup()
{
	// Initialise Serial
	Serial.begin(115200);

#ifdef ENABLE_OLED
	// Reset the OLED
	{
		gpio_set_direction(GPIO_NUM_16, gpio_mode_t::GPIO_MODE_OUTPUT);
		gpio_set_level(GPIO_NUM_16, 0);
		delay(41);
		gpio_set_level(GPIO_NUM_16, 1);
		delay(41);
	}
#endif

	// Initialise I2C
	{
		auto & i2c = I2C::X();
		i2c.init();
		auto foundDevices = i2c.scan();
		printf("Found I2C devices : ");
		for(const auto & device : foundDevices) {
			printf("%#02x ", device);
		}
		printf("\n");
	}

	// Initialise devices
	as5047.init();
	motorDriver.init();
	ina219.init();

	// Perform encoder calibration
	encoderCalibration.calibrate(as5047, motorDriver);
	xTaskCreate(&sendTorque, "sendTorque", 2048, NULL, 5, NULL);

#ifdef ENABLE_OLED
	u8g2_Setup_ssd1306_i2c_128x64_noname_1(oled.getU8g2()
		, U8G2_R0
		, u8g2_byte_hw_i2c_esp32
		, u8g2_gpio_and_delay_esp32);
	u8x8_SetPin(oled.getU8x8(), U8X8_PIN_RESET, GPIO_NUM_16);
	u8x8_SetI2CAddress(oled.getU8x8(), 0x3c);
	oled.begin();
#endif
}

uint8_t stepIndex;
uint16_t currentPosition;





void loop()
{
	//delay(1);
	//printf("\n");
	//ets_delay_us(50);
	//driveController.applyTorque(64, false);
	


}


// core 1 - 
// 	applyTorque function
// core 0 -
// 	print some stuff to screen (position, velocity)
// 	try different current values
// 	try different phase offsets