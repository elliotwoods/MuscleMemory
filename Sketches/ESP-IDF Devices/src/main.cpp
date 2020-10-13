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

#define ENABLE_OLED

#ifdef ENABLE_OLED
U8G2 oled;
#endif

MotorDriver motorDriver;
AS5047 as5047;
EncoderCalibration encoderCalibration;
DriveController driveController(motorDriver, as5047, encoderCalibration);

INA219 ina219;

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
	motorDriver.setTorque(0, 0);
	ina219.init();

	// Perform encoder calibration
	//encoderCalibration.calibrate(as5047, motorDriver);

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

#ifdef ENABLE_OLED
void draw() {
	oled.drawCircle(64,32,20);
	oled.drawLine(64,32,64,15);
}
#endif


void loop()
{
	delay(10);
	//printf("\n");

	int number = 5000;
	int list[] = {16,-32,64,-96,127,-255};
	for(int l=0;l<6;l++){
		printf("Running at %d torque\n", list[l]);
		printf("------------new Torque: %d \n", list[l]);
		for(int i=0;i<number;i++){
			driveController.applyTorque(list[l],i==0||i==number-1);
		}
		delay(1000);
	}
	delay(3000);

#ifdef ENABLE_OLED
	oled.firstPage();
	do {
		draw();
	} while (oled.nextPage());
#endif
}