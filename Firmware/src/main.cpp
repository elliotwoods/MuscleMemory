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
#include "Control/Provisioning.h"
#include "Control/FilteredTarget.h"

#include "GUI/Controller.h"
#include "GUI/Panels/RegisterList.h"
#include "GUI/Panels/SplashScreen.h"
#include "GUI/Panels/ShowID.h"
#include "GUI/Panels/Dashboard.h"

#include "Interface/SystemInfo.h"
#include "Interface/CANResponder.h"
#include "Interface/WebSockets.h"

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
#define PID_INSIDE_DRIVE_LOOP
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
Control::FilteredTarget filteredTarget;
Control::PID pid(filteredTarget);
Control::Drive drive(motorDriver, as5047, encoderCalibration, multiTurn);

Interface::SystemInfo systemInfo(ina219);
Interface::CANResponder canResponder(filteredTarget);
Interface::WebSockets webSockets(encoderCalibration, filteredTarget);

auto splashScreen = std::make_shared<GUI::Panels::SplashScreen>();

// Core 0 tasks:
#define PRIORITY_INTERFACE 0
#define PRIORITY_AGENT_SERVER_COMMS 0
#define PRIORITY_CAN_RESPONDER 0

// Core 1 tasks:
#define PRIORITY_MOTOR 1
#define PRIORITY_AGENT 1

bool fastBoot = false;

//----------
void showSplashMessage(const std::string & message)
{
	printf(message.c_str());
	printf("\n");

	uint16_t _minDisplayTime = 300;
	splashScreen->setMessage(message);
	if(!fastBoot) {
		vTaskDelay(_minDisplayTime / portTICK_PERIOD_MS);
	}
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
	Devices::I2C::X().init();

	// Init the registry
	Registry::X();

	// Initilaise the GUI
	GUI::Controller::X().init(splashScreen);

	// Show ESP-IDF version
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
	fastBoot = getRegisterValue(Registry::RegisterType::FastBoot) == 1;

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
#ifdef PID_INSIDE_DRIVE_LOOP
		if(getRegisterValue(Registry::RegisterType::ControlMode) == 1) {
			pid.update();
		}
#endif
		drive.update();
	}
}

//----------
void
agentTask(void*)
{
	while(true) {
		switch(getRegisterValue(Registry::RegisterType::ControlMode)) {
			case 1:
#ifndef PID_INSIDE_DRIVE_LOOP
				// This needs cleaning up
				pid.update();
#endif
				break;
#ifdef AGENT_ENABLED
			case 2:
				agent.update();
				break;
#endif
			default:
				break;
		}
		vTaskDelay(1 / portTICK_PERIOD_MS);
	}
}

//----------
void
canResponderTask(void*)
{
	while(true) {
		canResponder.updateTask();
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
	showSplashMessage("Initialising CAN");
	canResponder.init();

#ifdef WEBSOCKETS_ENABLED
	showSplashMessage("Initialising websockets");
	webSockets.init();
#endif

#ifdef PROVISIONING_ENABLED
	if(Registry::X().registers.at(Registry::RegisterType::ProvisioningEnabled).value) {
		Control::Provisioning provisioning(motorDriver, ina219, as5047, encoderCalibration, canResponder, webSockets);
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
		if(!encoderCalibration.calibrate(as5047, motorDriver)) {
			abort();
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

	showSplashMessage("Starting system tasks");

	xTaskCreatePinnedToCore(motorTask
		, "Motor"
		, 1024 * 4
		, NULL
		, PRIORITY_MOTOR
		, NULL
		, 1);

#if !defined(PID_INSIDE_DRIVE_LOOP) || defined(AGENT_ENABLED)
	xTaskCreatePinnedToCore(agentTask
		, "Agent"
		, 1024 * 8
		, NULL
		, PRIORITY_AGENT
		, NULL
		, 0);
#endif

#ifdef AGENT_ENABLED
	xTaskCreatePinnedToCore(agentServerCommunicateTask
		, "AgentServer"
		, 1024 * 4
		, NULL
		, PRIORITY_AGENT_SERVER_COMMS
		, NULL 
		, 0);
#endif
	xTaskCreatePinnedToCore(canResponderTask
		, "CANResponder"
		, 1024 * 4
		, NULL
		, PRIORITY_CAN_RESPONDER
		, NULL
		, 0);

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
	filteredTarget.update();
#ifdef WEBSOCKETS_ENABLED
	webSockets.update();
#endif
	GUI::Controller::X().update();
	const auto & delay = Registry::X().registers.at(Registry::RegisterType::MainLoopDelay).value;
	vTaskDelay(delay / portTICK_PERIOD_MS);
}

//----------
void
interfaceTask(void*)
{
	while(true) {
		if(getRegisterValue(Registry::RegisterType::Reboot) == 1) {
			esp_restart();
		}
		updateInterface();
	}
}

//----------
void
initInterface()
{
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