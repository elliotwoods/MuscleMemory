#include "MotorDriver.h"

namespace Devices {
	//----------
	MotorDriver::Configuration::Configuration()
	{
		this->coilPins.coil_A_positive = GPIO_NUM_32;
		this->coilPins.coil_B_positive = GPIO_NUM_14;
		this->coilPins.coil_A_negative = GPIO_NUM_33;
		this->coilPins.coil_B_negative = GPIO_NUM_27;

		this->vrefDacs.A = DAC_GPIO25_CHANNEL;
		this->vrefDacs.B = DAC_GPIO26_CHANNEL;
	}

	//----------
	void
	MotorDriver::init(const MotorDriver::Configuration & configuration)
	{
		this->configuration = configuration;

		// Initalise GPIO digital outputs (Coils)
		for(uint8_t i=0; i<4; i++) {
			const auto & coilPin = this->configuration.coilPinArray[i];
			gpio_reset_pin(coilPin);
			gpio_set_direction(coilPin, GPIO_MODE_OUTPUT);
			gpio_set_level(coilPin, 0);
		}

		// Initialise DAC outputs (VREFs)
		for(uint8_t i=0; i<2; i++) {
			const auto & dac = this->configuration.vrefDacArray[i];
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
	MotorDriver::setTorque(Torque torque, PositionWithinStepCycle positionWithinStepCycle)
	{
		bool positiveDirection = torque >= 0;

		//presume position is 0 to start and we're moving positive

		int8_t coil_A;
		int8_t coil_B;

		if(positiveDirection) {
			coil_A = this->cosTableFull[positionWithinStepCycle + (uint8_t) 64];
			coil_B = this->cosTableFull[positionWithinStepCycle];
		}
		else {
			coil_A = this->cosTableFull[positionWithinStepCycle - (uint8_t) 64];
			coil_B = this->cosTableFull[positionWithinStepCycle + (uint8_t) 128];
		}

		//printf("\t Torque: \t %d", torque);
		//printf("\t Coils: \t %d, %d\n", coil_A, coil_B);

		// Coil A
		{
			uint8_t referenceVoltage = uint8_t (((uint16_t) abs(coil_A) * (uint16_t) abs(torque)) / (uint16_t) (64));
			dac_output_voltage(this->configuration.vrefDacs.A, referenceVoltage);
			//printf("Coil A reference voltage: %d\n", referenceVoltage);
		}

		// Coil B
		{
			uint8_t referenceVoltage = uint8_t (((uint16_t) abs(coil_B) * (uint16_t) abs(torque)) / (uint16_t) (64));
			dac_output_voltage(this->configuration.vrefDacs.B, referenceVoltage);
			//printf("Coil B reference voltage: %d\n", referenceVoltage);
		}

		gpio_set_level(this->configuration.coilPins.coil_A_positive, coil_A > 0);
		gpio_set_level(this->configuration.coilPins.coil_A_negative, coil_A < 0);
		gpio_set_level(this->configuration.coilPins.coil_B_positive, coil_B > 0);
		gpio_set_level(this->configuration.coilPins.coil_B_negative, coil_B < 0);
	}

	//----------
	void
	MotorDriver::step(uint8_t index, uint8_t current)
	{
		for(uint8_t i=0; i<4; i++) {
			gpio_set_level(this->configuration.coilPinArray[i], i == index);
		}

		for(uint8_t i=0; i<2; i++) {
			dac_output_voltage(this->configuration.vrefDacArray[i], current);
		}
	}
}
