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
#include "Control/WebSockets.h"
#include "Control/Provisioning.h"

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

#ifdef Arduino
#include "FreeRTOS.h"
#else
#include <freertos/FreeRTOS.h>
#endif

#include "driver/can.h"

//#define AGENT_ENABLED
#define WEBSOCKETS_ENABLED
#define PROVISIONING_ENABLED

#if defined(AGENT_ENABLED) || defined(WEBSOCKETS_ENABLED)
#define WIFI_ENABLED
#endif


Devices::MotorDriver motorDriver;
Devices::AS5047 as5047; // The magnetic encoder
Devices::INA219 ina219; // The current sensor
Devices::FileSystem fileSystem;

Control::EncoderCalibration encoderCalibration;
Control::MultiTurn multiTurn(encoderCalibration);
#ifdef AGENT_ENABLED
Control::Agent agent;
#endif
Control::PID pid;
Control::Drive drive(motorDriver, as5047, encoderCalibration, multiTurn);

#ifdef WEBSOCKETS_ENABLED
Control::WebSockets webSockets;
#endif

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
	printf(message.c_str());
	printf("\n");

	uint16_t _minDisplayTime = 300;
	splashScreen->setMessage(message);
	vTaskDelay(_minDisplayTime / portTICK_PERIOD_MS);
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

	// Print IDF version
	{
		char message[100];
		sprintf(message, "ESP-IDF : %s", esp_get_idf_version());
		showSplashMessage(message);
	}

	// Initialise devices
	showSplashMessage("Mount File System...");	
	fileSystem.mount("appdata", "/appdata", true, 2);

	showSplashMessage("Load registry defaults...");	
	Registry::X().loadDefaults();

#ifdef AGENT_ENABLED
	// Agent based
	Registry::X().registers.at(Registry::RegisterType::ControlMode).value = 2;
#else
	// PID
	Registry::X().registers.at(Registry::RegisterType::ControlMode).value = 1;
#endif

	showSplashMessage("Initialise AS5047...");
	as5047.init();
	
	showSplashMessage("Initialise A4954...");
	motorDriver.init();

	showSplashMessage("Initialise INA219...");
	ina219.init();	

#ifdef WIFI_ENABLED
	showSplashMessage("Connect to WiFi ...");
	Devices::Wifi::X().init();
#endif
}

//----------
void
motorTask(void*)
{
	while(true) {
		drive.update();
		vTaskDelay(1);
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
	//timerAlarmEnable(timer);

	const auto & controlMode = Registry::X().registers.at(Registry::RegisterType::ControlMode).value;

	while(true) {
		if(xSemaphoreTake(agentTaskResumeMutex, portMAX_DELAY)) {
			switch(controlMode) {
				case 1:
					pid.update();
					break;
#ifdef AGENT_ENABLED
				case 2:
					agent.update();
					break;
#endif
				default:
					break;
			}
			xSemaphoreGive(agentTaskResumeMutex);
		}
		vTaskDelay(10);
		//vTaskSuspend(agentTaskHandle);
	}
}

#ifdef AGENT_ENABLED
//----------
void
agentServerCommunicateTask(void*)
{
	agent.serverCommunicateTask();
}
#endif

//----------
void
initController()
{
#ifdef PROVISIONING_ENABLED
	if(Registry::X().registers.at(Registry::RegisterType::NeedsProvisioning).value) {
		Control::Provisioning provisioning(motorDriver, ina219, as5047, encoderCalibration);
		provisioning.perform();
	}
#endif

	// Load calibration for encoder, or perform one if none available
	if(encoderCalibration.load()) {
		showSplashMessage("Encoder calibration loaded");
	}
	else {
		// Perform encoder calibration
		showSplashMessage("Calibrating encoder");
		if(encoderCalibration.calibrate(as5047, motorDriver)) {
			if(!encoderCalibration.save()) {
				abort();
			}
			printf("Motor calibration saved\n");
		}
	}

	showSplashMessage("Initialise MultiTurn");
	multiTurn.init(as5047.getPosition());

	// Initialise target to initial reading
	Registry::X().registers.at(Registry::RegisterType::TargetPosition).value = multiTurn.getMultiTurnPosition();

#ifdef AGENT_ENABLED
	showSplashMessage("Initialise Agent");
	agent.init();
#endif

	showSplashMessage("Initialise Drive");
	drive.init();

	showSplashMessage("Initialise PID");
	pid.init();

#ifdef WEBSOCKETS_ENABLED
	showSplashMessage("Initialising websockets");
	webSockets.init();
#endif

	showSplashMessage("Starting system tasks");

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

#ifdef AGENT_ENABLED
	xTaskCreatePinnedToCore(agentServerCommunicateTask
		, "AgentServer"
		, 1024 * 4
		, NULL
		, PRIORITY_AGENT_SERVER_COMMS
		, NULL 
		, 0);
#endif

	showSplashMessage("Controller initialised");
}

//----------
void
updateInterface()
{
	Registry::X().update();
	multiTurn.mainLoopUpdate();
	systemInfo.update();
	canResponder.update();
#ifdef WEBSOCKETS_ENABLED
	webSockets.update();
#endif
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

	// Display ID
	{
		auto showID = std::make_shared<GUI::Panels::ShowID>();
		GUI::Controller::X().setRootPanel(showID);
		while(!showID->shouldExit) {
			GUI::Controller::X().update();
			vTaskDelay(10 / portTICK_PERIOD_MS);
		}
	}
	
	GUI::Controller::X().setRootPanel(std::make_shared<GUI::Panels::Dashboard>());
	initInterface();
}

//----------
void
loop()
{
	vTaskDelay(500 / portTICK_PERIOD_MS);
}

#endif