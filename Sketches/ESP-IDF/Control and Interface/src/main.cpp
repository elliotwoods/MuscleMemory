#include <Arduino.h>

#include "Devices/AS5047.h"
#include "Devices/MotorDriver.h"
#include "Devices/I2C.h"
#include "Devices/INA219.h"

#include "Control/EncoderCalibration.h"
#include "Control/Drive.h"

#include "GUI/Controller.h"
#include "GUI/Panels/RegisterList.h"

Devices::MotorDriver motorDriver;
Devices::AS5047 as5047;
Devices::INA219 ina219;

Control::EncoderCalibration encoderCalibration;
Control::Drive drive(motorDriver, as5047, encoderCalibration);

//----------
void
initDevices()
{
	// Initialise Serial
	Serial.begin(115200);

	// Initialise I2C
	{
		auto & i2c = Devices::I2C::X();
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
}

//----------
void
initController()
{
	// Perform encoder calibration
	const bool performCalibration = true;
	if(performCalibration) {
		encoderCalibration.calibrate(as5047, motorDriver);
		{
			uint16_t position = 0;
			encoderCalibration.save(position);
		}
	}
	else {
		uint16_t position = 0;
		encoderCalibration.load(position);
	}
}

//----------
void
updateInterface()
{
	GUI::Controller::X().update();
	//Network::processMessages();
}

//----------
void
runInterface(void*)
{
	while(true) {
		vTaskDelay(10 / portTICK_PERIOD_MS);
		updateInterface();
	}
}

//----------
void
initInterface()
{
	GUI::Controller::X().init(std::make_shared<GUI::Panels::RegisterList>());
	xTaskCreatePinnedToCore(runInterface
		, "Interface"
		, 10000
		, NULL
		, 1
		, NULL
		, 0);
}

//----------
void controlLoop()
{
	drive.applyTorque(4, false);
}

#ifdef ARDUINO

//----------
void
setup()
{
	initDevices();
	initController();
	initInterface();
}

//----------
void
loop()
{
	controlLoop();
}

#endif