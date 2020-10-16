#include "Devices/AS5047.h"
#include "Devices/MotorDriver.h"
#include "Devices/I2C.h"
#include "Devices/INA219.h"
#include "Devices/FileSystem.h"
#include "Devices/Wifi.h"

#include "Control/EncoderCalibration.h"
#include "Control/MultiTurn.h"
#include "Control/Agent.h"
#include "Control/Drive.h"

#include "GUI/Controller.h"
#include "GUI/Panels/RegisterList.h"

#include "Interface/SystemInfo.h"

#include "Registry.h"
#include "WifiConfig.h"

#include "cJSON.h"

Devices::MotorDriver motorDriver;
Devices::AS5047 as5047;
Devices::INA219 ina219;
Devices::FileSystem fileSystem;

Control::EncoderCalibration encoderCalibration;
Control::MultiTurn multiTurn(encoderCalibration);
Control::Agent agent;
Control::Drive drive(motorDriver, as5047, encoderCalibration, multiTurn, agent);

Interface::SystemInfo systemInfo(ina219);

//----------
void
initDevices()
{
	// Initialise Serial
	{
		// For now we use Arduino - we change this later
		Serial.begin(115200);
	}

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
	fileSystem.mount("appdata", "/appdata", true, 2);
	Devices::Wifi::X().init(MUSCLE_MEMORY_SERVER);
}

//----------
void
initController()
{
	if(encoderCalibration.load()) {
		printf("Motor calibration loaded\n");
	}
	else {
		// Perform encoder calibration
		encoderCalibration.calibrate(as5047, motorDriver);
		if(!encoderCalibration.save()) {
			abort();
		}
		printf("Motor calibration saved\n");
	}

	multiTurn.init(as5047.getPosition());
	agent.init();
	drive.init();
}

//----------
void
updateInterface()
{
	Registry::X().update();
	systemInfo.update();
	GUI::Controller::X().update();
	agent.update();
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
	systemInfo.init();

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
	drive.update();
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