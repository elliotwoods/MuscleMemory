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

#include "Utils/Scheduler.h"

#include "Registry.h"
#include "WifiConfig.h"

#include "cJSON.h"

#include "FreeRTOS.h"

#include "driver/can.h"

Devices::MotorDriver motorDriver;
Devices::AS5047 as5047;
Devices::INA219 ina219;
Devices::FileSystem fileSystem;

Control::EncoderCalibration encoderCalibration;
Control::MultiTurn multiTurn(encoderCalibration);
Control::Agent agent;
Control::Drive drive(motorDriver, as5047, encoderCalibration, multiTurn, agent);

Interface::SystemInfo systemInfo(ina219);

TaskHandle_t agentTaskHandle;
SemaphoreHandle_t agentTaskResumeMutex;

// Core 0 tasks:
#define PRIORITY_INTERFACE 1
#define PRIORITY_AGENT_SERVER_COMMS 2

// Core 1 tasks:
#define PRIORITY_MOTOR 2
#define PRIORITY_AGENT 1

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
motorTask(void*)
{
	while(true) {
		drive.update();
	}
}

//----------
void
wakeAgent()
{
	BaseType_t taskWoken;
	if(xSemaphoreTakeFromISR(agentTaskResumeMutex, &taskWoken)) {
		vTaskResume(agentTaskHandle);
		xSemaphoreGiveFromISR(agentTaskResumeMutex, &taskWoken);
	}
}

//----------
void
agentTask(void*)
{
	// Scheduler isn't working
	//Utils::Scheduler::X().schedule(0.01f, wakeAgent);

	// Use an Arduino ESP32 timer
	auto timer = timerBegin(0, 16, true);
	timerAttachInterrupt(timer, wakeAgent, true);
	timerAlarmWrite(timer, 5000, true);
	timerAlarmEnable(timer);

	agentTaskResumeMutex = xSemaphoreCreateMutex();
	while(true) {
		if(xSemaphoreTake(agentTaskResumeMutex, portMAX_DELAY)) {
			agent.update();
			xSemaphoreGive(agentTaskResumeMutex);
		}
		vTaskSuspend(agentTaskHandle);
	}
}

//----------
void
agentServerCommunicateTask(void*)
{
	agent.serverCommunicateTask();
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

	xTaskCreatePinnedToCore(motorTask
		, "Motor"
		, 1024 * 4
		, NULL
		, PRIORITY_MOTOR
		, NULL
		, 1);
	
	xTaskCreatePinnedToCore(agentTask
		, "Agent"
		, 1024 * 8
		, NULL
		, PRIORITY_AGENT
		, &agentTaskHandle
		, 1);
	
	xTaskCreatePinnedToCore(agentServerCommunicateTask
		, "AgentServer"
		, 1024 * 4
		, NULL
		, PRIORITY_AGENT_SERVER_COMMS
		, NULL
		, 0);
}

//----------
void
updateInterface()
{
	Registry::X().update();
	systemInfo.update();
	GUI::Controller::X().update();
	//Network::processMessages();
}

//----------
void
interfaceTask(void*)
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

	xTaskCreatePinnedToCore(interfaceTask
		, "Interface"
		, 4096
		, NULL
		, PRIORITY_INTERFACE
		, NULL
		, 0);
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
	vTaskDelay(500 / portTICK_PERIOD_MS);
}

#endif