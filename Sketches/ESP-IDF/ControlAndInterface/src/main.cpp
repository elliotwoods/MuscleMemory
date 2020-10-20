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
#include "Control/PID.h"

#include "GUI/Controller.h"
#include "GUI/Panels/RegisterList.h"
#include "GUI/Panels/SplashScreen.h"
#include "GUI/Panels/ShowID.h"
#include "GUI/Panels/Dashboard.h"

#include "Interface/SystemInfo.h"
#include "Interface/CANResponder.h"

#include "Utils/Scheduler.h"

#include "Registry.h"
#include "WifiConfig.h"

#include "cJSON.h"

#include "FreeRTOS.h"

#include "driver/can.h"

Devices::MotorDriver motorDriver;
Devices::AS5047 as5047; // The magnetic encoder
Devices::INA219 ina219; // The current sensor
Devices::FileSystem fileSystem;

Control::EncoderCalibration encoderCalibration;
Control::MultiTurn multiTurn(encoderCalibration);
Control::Agent agent;
Control::PID pid;
Control::Drive drive(motorDriver, as5047, encoderCalibration, multiTurn);

Interface::SystemInfo systemInfo(ina219);
Interface::CANResponder canResponder;

TaskHandle_t agentTaskHandle;
SemaphoreHandle_t agentTaskResumeMutex;

auto splashScreen = std::make_shared<GUI::Panels::SplashScreen>();

// Core 0 tasks:
#define PRIORITY_INTERFACE 1
#define PRIORITY_AGENT_SERVER_COMMS 2

// Core 1 tasks:
#define PRIORITY_MOTOR 2
#define PRIORITY_AGENT 1

//----------
void showSplashMessage(const std::string & message)
{
	splashScreen->setMessage(message);
	GUI::Controller::X().update();
}

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

	// Initilaise the GUI here!
	GUI::Controller::X().init(splashScreen);


	// Initialise devices
	uint16_t _minDisplayTime = 300;
	showSplashMessage("Initialise AS5047...");
	as5047.init();
	vTaskDelay(_minDisplayTime / portTICK_PERIOD_MS);
	showSplashMessage(" done.");
	

	showSplashMessage("Initialise A4954ELP...");
	motorDriver.init();
	vTaskDelay(_minDisplayTime / portTICK_PERIOD_MS);
	showSplashMessage(" done.");

	showSplashMessage("Initialise INA219...");
	ina219.init();	
	vTaskDelay(_minDisplayTime / portTICK_PERIOD_MS);
	showSplashMessage(" done.");

	showSplashMessage("Mount File System...");	
	fileSystem.mount("appdata", "/appdata", true, 2);
	vTaskDelay(_minDisplayTime / portTICK_PERIOD_MS);
	showSplashMessage(" done.");

	showSplashMessage("Connect to Server...");
	//Devices::Wifi::X().init(MUSCLE_MEMORY_SERVER);
	vTaskDelay(_minDisplayTime / portTICK_PERIOD_MS);
	showSplashMessage(" done.");



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

	agentTaskResumeMutex = xSemaphoreCreateMutex();

	// Use an Arduino ESP32 timer
	auto timer = timerBegin(0, 16, true);
	timerAttachInterrupt(timer, wakeAgent, true);
	timerAlarmWrite(timer, 5000, true);
	timerAlarmEnable(timer);

	const auto & controlMode = Registry::X().registers.at(Registry::RegisterType::ControlMode).value;

	while(true) {
		if(xSemaphoreTake(agentTaskResumeMutex, portMAX_DELAY)) {
			switch(controlMode) {
				case 1:
					pid.update();
					break;
				case 2:
					agent.update();
					break;
				default:
					break;
			}
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
	//agent.init();
	drive.init();
	pid.init();
	
	xTaskCreatePinnedToCore(motorTask
		, "Motor"
		, 1024 * 4
		, NULL
		, PRIORITY_MOTOR
		, NULL
		, 1);

	// xTaskCreatePinnedToCore(agentTask
	// 	, "Agent"
	// 	, 1024 * 8
	// 	, NULL
	// 	, PRIORITY_AGENT
	// 	, &agentTaskHandle
	// 	, 1);

	// xTaskCreatePinnedToCore(agentServerCommunicateTask
	// 	, "AgentServer"
	// 	, 1024 * 4
	// 	, NULL
	// 	, PRIORITY_AGENT_SERVER_COMMS
	// 	, NULL 
	// 	, 0);
}

//----------
void
updateInterface()
{
	Registry::X().update();
	systemInfo.update();
	canResponder.update();
	GUI::Controller::X().update();
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
	systemInfo.init();
	canResponder.init();

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
	
	GUI::Controller::X().setRootPanel(std::make_shared<GUI::Panels::ShowID>());
	GUI::Controller::X().update();
	
	vTaskDelay(2500 / portTICK_PERIOD_MS);
	
	GUI::Controller::X().setRootPanel(std::make_shared<GUI::Panels::RegisterList>());
	//GUI::Controller::X().setRootPanel(std::make_shared<GUI::Panels::Dashboard>());
	initInterface();
	//Registry::X().registers.at(Registry::RegisterType::Torque).value = 16;

}

//----------
void
loop()
{
	vTaskDelay(500 / portTICK_PERIOD_MS);
}

#endif