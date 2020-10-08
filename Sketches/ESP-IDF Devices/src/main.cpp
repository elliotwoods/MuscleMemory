#include <Arduino.h>
#include "sine_stepper.h"
#include "AS5047.h"
#include "MotorDriver.h"
#include "U8g2lib.h"
#include "I2C.h"
#include "INA219.h"

AS5047 encoder;

//#define ENABLE_OLED

#ifdef ENABLE_OLED
U8G2_SSD1306_128X64_NONAME_1_SW_I2C oled(U8G2_R0, 15, 4, 16);
#endif

MotorDriver motor;
INA219 ina219;

void setup()
{
	Serial.begin(115200);

	while (!Serial) {
		delay(100);
	}
	Serial.println("Serial initialised");

	// motor_setup(32, 33, 14, 27, 200);
	// set_speed(20);

	// for (int i = 0; i < 2; i++)
	// {
	// 	step(10);
	// 	delay(1000);
	// 	//step(10);
	// 	//delay(1000);
	// 	step(-13);
	// 	delay(1000);
	// 	//step(-7);
	// 	//delay(1000);
	// }

	MotorDriver::Configuration configuration;
	{
		configuration.coilPins = MotorDriver::Configuration::CoilPins{ GPIO_NUM_32, GPIO_NUM_14, GPIO_NUM_33, GPIO_NUM_27};
		configuration.vrefDACs = MotorDriver::Configuration::VREFDACs{ DAC_GPIO25_CHANNEL, DAC_GPIO26_CHANNEL };
	}
	motor.setup(configuration);
	motor.setTorque(0, 0);

#ifdef ENABLE_OLED
	oled.begin();
#endif

	// Reset the OLED
	{
		gpio_set_direction(GPIO_NUM_16, gpio_mode_t::GPIO_MODE_OUTPUT);
		gpio_set_level(GPIO_NUM_16, 0);
		delay(10);
		gpio_set_level(GPIO_NUM_16, 1);
	}

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

	ina219.init(INA219::Configuration());
}

uint8_t stepIndex;
uint16_t currentPosition;

#ifdef ENABLE_OLED
void draw() {
	oled.drawCircle(64,32,20);
	oled.drawLine(64,32,64,15);
	oled.setFont(u8g2_font_fub11_tf);

	uint16_t y = 1;
	{
		char message[100];
		sprintf(message, "Enc : %d", currentPosition);
		oled.drawStr(0,y++ * 16,message);
	}

	{
		char message[100];
		sprintf(message, "Error : %d", encoder.getErrors());
		oled.drawStr(0, y++ * 16, message);
	}

	
	{
		char message[100];
		sprintf(message, "Step : %d", stepIndex);
		oled.drawStr(0, y++ * 16, message);
	}
}
#endif

void loop()
{
	return;

	//motor.setTorque(1, 0);
	//delay(100);
	//motor.setTorque(0, 0);
	
	motor.step(stepIndex, 64);
	stepIndex = (stepIndex + 1) % 4;

	delay(10);

	currentPosition = encoder.getPosition();
	printf("pos : %d \n", currentPosition);
	{
		auto flipped = ((currentPosition & 255) << 8) | ((currentPosition >> 8) & 255);
		printf("flp : %d \n", flipped);
	}

	for(int i=15; i>=0; i--) {
		printf((currentPosition >> i) & 1 ? "1" : "0");
	}
	printf("\n");

	{
		auto errors = encoder.getErrors();
		if(errors != 0) {
			Serial.println("Some error");

			if(errors & AS5047::Errors::framingError) {
				Serial.println("Framing error");
			
			}if(errors & AS5047::Errors::invalidCommand) {
				Serial.println("Invalid command");
			}
			if(errors & AS5047::Errors::parityError) {
				Serial.println("Parity error");
			}
		}
	}

#ifdef ENABLE_OLED
	oled.firstPage();
	do {
		draw();
	} while (oled.nextPage());
#endif
}