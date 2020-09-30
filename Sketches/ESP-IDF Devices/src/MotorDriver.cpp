#include "MotorDriver.h"

//----------
MotorDriver::MotorDriver(const Configuration & configuration)
: configuration(configuration)
{
	// Initalise GPIO digital outputs (Coils)
	for(uint8_t i=0; i<4; i++) {
		const auto & pin = this->configuration.pinArray[i];
		gpio_reset_pin(pin);
		gpio_set_direction(pin, GPIO_MODE_OUTPUT);
		gpio_set_level(pin, 0);
	}

	// Initialise DAC outputs (VREFs)
	for(uint8_t i=0; i<2; i++) {
		const auto & dac = this->configuration.dacArray[i];
		dac_output_enable(dac);
		dac_output_voltage(dac, 0);
	}

	// Calculate sine table values
	this->calculateCosTable();
}

#define CYCLE_RESOLUTION 256

//----------
void
MotorDriver::calculateCosTable()
{
	for(uint16_t i=0; i<256; i++)
	{
		double value = cos((double)PI * i / CYCLE_RESOLUTION);
		this->cosTable[i] = 255 * value;

		this->cosTableFull[i] = (int8_t) (127.0 * cos((double)PI * 2 * i / CYCLE_RESOLUTION));
	}
}

//----------
void
MotorDriver::setTorque(int8_t torque, uint8_t cyclePosition)
{
	bool positiveDirection = torque >= 0;

	//presume position is 0 to start and we're moving positive

	int8_t coil_A;
	int8_t coil_B;

	if(positiveDirection) {
		coil_A = this->cosTableFull[cyclePosition + (uint8_t) 64];
		coil_B = this->cosTableFull[cyclePosition];
	}
	else {
		coil_A = this->cosTableFull[cyclePosition - (uint8_t) 64];
		coil_B = this->cosTableFull[cyclePosition + (uint8_t) 128];
	}

	// Coil A
	{
		uint8_t voltage = uint8_t (((uint16_t) abs(coil_A) * (uint16_t) abs(torque)) / (uint16_t) (256 * 128));
		dac_output_voltage(this->configuration.vrefDACs.A, voltage);
	}

	// Coil B
	{
		uint8_t voltage = uint8_t (((uint16_t) abs(coil_B) * (uint16_t) abs(torque)) / (uint16_t) (256 * 128));
		dac_output_voltage(this->configuration.vrefDACs.B, voltage);
	}

	gpio_set_level(this->configuration.coilPins.coil_A_positive, coil_A >= 0);
	gpio_set_level(this->configuration.coilPins.coil_A_negative, coil_A < 0);
	gpio_set_level(this->configuration.coilPins.coil_B_positive, coil_B >= 0);
	gpio_set_level(this->configuration.coilPins.coil_B_negative, coil_B < 0);
}
