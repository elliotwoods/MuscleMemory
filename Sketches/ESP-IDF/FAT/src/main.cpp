#include <Arduino.h>

#include "Devices/AS5047.h"
#include "Devices/MotorDriver.h"
#include "Devices/I2C.h"
#include "Devices/INA219.h"

#include "GUI/Controller.h"
#include <esp_partition.h>
#include "FATTest.h"

Devices::MotorDriver motorDriver;
Devices::AS5047 as5047;
Devices::INA219 ina219;

class TestPanel : public GUI::Panel {
public:
	void update() override {

	}
	void draw(U8G2 & oled) {
		oled.setFont(u8g2_font_nerhoe_tr);
		oled.drawStr(0, 10, "Hello!");
	};
	bool buttonPressed() {
		return false;
	}
	void dial(int8_t) {}
private:
	uint32_t dummy;
};
 

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
	GUI::Controller::X().init(std::make_shared<TestPanel>());
	xTaskCreatePinnedToCore(runInterface
		, "Interface"
		, 10000
		, NULL
		, 1
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
	fatTest();

}

//----------
void
loop()
{
	delay(10);
}

#endif