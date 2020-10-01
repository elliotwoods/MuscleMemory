#include <Arduino.h>
#include "sine_stepper.h"
#include "AS5047.h"
#include "MotorDriver.h"

AS5047 encoder;


MotorDriver motor;

void setup()
{
	Serial.begin(115200);

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
}

uint8_t stepIndex;

void loop()
{
	//motor.setTorque(1, 0);
	//delay(100);
	//motor.setTorque(0, 0);
	motor.step(stepIndex, 64);
	stepIndex = (stepIndex + 1) % 4;

	delay(10);
	Serial.println(encoder.getPosition());

	{
		auto errors = encoder.getErrors();
		if(errors.hasErrors) {
			if(errors.framingError) {
				Serial.println("Framing error");
			
			}if(errors.invalidCommand) {
				Serial.println("Invalid command");
			}
			if(errors.parityError) {
				Serial.println("Parity error");
			}
		}
	}
}