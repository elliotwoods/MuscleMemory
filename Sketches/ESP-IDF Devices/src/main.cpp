#include <Arduino.h>
#include "sine_stepper.h"
#include "AS5047.h"
#include "MotorDriver.h"
#include "U8g2lib.h"
#include "I2C.h"
#include "INA219.h"
#include "EncoderCalibration.h"
#include "DriveController.h"


//#define ENABLE_OLED

#ifdef ENABLE_OLED
U8G2_SSD1306_128X64_NONAME_1_SW_I2C oled(U8G2_R0, 15, 4, 16);
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
	encoderCalibration.calibrate(as5047, motorDriver);

#ifdef ENABLE_OLED
	oled.begin();

	// Reset the OLED
	{
		gpio_set_direction(GPIO_NUM_16, gpio_mode_t::GPIO_MODE_OUTPUT);
		gpio_set_level(GPIO_NUM_16, 0);
		delay(10);
		gpio_set_level(GPIO_NUM_16, 1);
	}
#endif
}

uint8_t stepIndex;
uint16_t currentPosition;

#ifdef ENABLE_OLED
void draw() {
	oled.drawCircle(64,32,20);
	oled.drawLine(64,32,64,15);
	as5047.drawDebug(oled);
}
#endif

void loop()
{
	//delay(10);
	//printf("\n");

	driveController.applyTorque(16);

#ifdef ENABLE_OLED
	oled.firstPage();
	do {
		draw();
	} while (oled.nextPage());
#endif
}