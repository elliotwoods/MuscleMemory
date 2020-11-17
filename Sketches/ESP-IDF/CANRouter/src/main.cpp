#include "Devices/I2C.h"
#include "Devices/INA219.h"
#include "Devices/FileSystem.h"

#include "GUI/Controller.h"
#include "GUI/Panels/SplashScreen.h"
#include "GUI/Panels/Lambda.h"

#include "Utils/Scheduler.h"

#include "Tasks.h"

#ifdef Arduino
#include "FreeRTOS.h"
#else
#include <freertos/FreeRTOS.h>
#endif

extern "C" {
	#include "driver/can.h"
	#include "esp_ota_ops.h"
}

Devices::INA219 ina219; // The current sensor
Devices::FileSystem fileSystem;

auto splashScreen = std::make_shared<GUI::Panels::SplashScreen>();

// Core 0 tasks:
#define PRIORITY_INTERFACE 0
#define PRIORITY_AGENT_SERVER_COMMS 0
#define PRIORITY_CAN_RESPONDER 1

// Core 1 tasks:
#define PRIORITY_MOTOR 1
#define PRIORITY_AGENT 1

bool fastBoot = false;
auto panel = std::make_shared<GUI::Panels::Lambda>();

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
		serialInit();
	}

	// Initialise I2C
	Devices::I2C::X().init();

	// Initilaise the GUI
	GUI::Controller::X().init(splashScreen);

	// Show ESP-IDF version
	{
		char message[100];
		sprintf(message, "ESP-IDF : %s", esp_get_idf_version());
		showSplashMessage(message);
	}


	// Show boot partition
	{
		char message[100];
		auto bootPartition = esp_ota_get_boot_partition();
		sprintf(message, "Booting to %s", bootPartition->label);
		showSplashMessage(message);
	}

	// Initialise CAN
	{
		showSplashMessage("Initialise CAN...");	
		canInit();
	}

	// Initialise devices
	showSplashMessage("Mount File System...");	
	fileSystem.mount("appdata", "/appdata", true, 2);

	showSplashMessage("Initialise INA219...");
	ina219.init();	
}

//----------
void
updateInterface()
{
	GUI::Controller::X().update();
	vTaskDelay(10 / portTICK_PERIOD_MS);
}

//----------
void
interfaceTask(void*)
{
	while(true) {
		updateInterface();
	}
}

//----------
void
initInterface()
{
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
	initInterface();

	xTaskCreatePinnedToCore(canTask
		, "CAN"
		, 8192
		, NULL
		, 1
		, NULL
		, 1);

	xTaskCreatePinnedToCore(serialTask
		, "Serial"
		, 8192
		, NULL
		, 1
		, NULL
		, 0);

	panel->onDraw = [&](U8G2& u8g2) {
		u8g2.setFont(u8g2_font_10x20_mr);

		char message[100];// Draw title
		{
			auto count = getCanToSerialCount();
			if(count > 0) {
				sprintf(message, "Rx : %d", count);
				u8g2.drawStr(5, 20, message);
			}
		}

		{
			auto count = getSerialToCanCount();
			if(count > 0) {
				sprintf(message, "Tx : %d", count);
				u8g2.drawStr(5, 40, message);
			}
		}

		u8g2.drawCircle(124, 4, (esp_timer_get_time() / 100000) % 4);
	};
	GUI::Controller::X().setRootPanel(panel);
}

//----------
void
loop()
{
	vTaskDelay(500 / portTICK_PERIOD_MS);
}

#endif