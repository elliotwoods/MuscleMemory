#include "MotorDriver.h"
#include "Registry.h"
#include "Platform/Platform.h"

#ifdef ARDUINO
	#define DAC_A DAC_GPIO25_CHANNEL
	#define DAC_B DAC_GPIO26_CHANNEL
#else
	#define DAC_A dac_channel_t::DAC_CHANNEL_1
	#define DAC_B dac_channel_t::DAC_CHANNEL_2
#endif

namespace Devices {
	//----------
	MotorDriver::Configuration::Configuration()
	{
		this->coilPins.coil_A_positive = MM_CONFIG_MOTOR_DRIVER_PIN_A_POSITIVE;
		this->coilPins.coil_B_positive = MM_CONFIG_MOTOR_DRIVER_PIN_B_POSITIVE;
		this->coilPins.coil_A_negative = MM_CONFIG_MOTOR_DRIVER_PIN_A_NEGATIVE;
		this->coilPins.coil_B_negative = MM_CONFIG_MOTOR_DRIVER_PIN_B_NEGATIVE;

		this->vrefDacs.A = MM_CONFIG_MOTOR_DRIVER_DAC_A;
		this->vrefDacs.B = MM_CONFIG_MOTOR_DRIVER_DAC_B;
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

#ifdef MM_CONFIG_MOTOR_DRIVER_ENABLE_ENABLED
		// Initialise drive enabled output
		gpio_reset_pin(MM_CONFIG_MOTOR_DRIVER_PIN_ENABLE);
		gpio_set_direction(MM_CONFIG_MOTOR_DRIVER_PIN_ENABLE, GPIO_MODE_OUTPUT);
#endif

		// Calculate sine table values
		this->calculateCosTable();
	}

	#define CYCLE_RESOLUTION 256

	//----------
	void
	MotorDriver::update()
	{
#ifdef MM_CONFIG_MOTOR_DRIVER_ENABLE_ENABLED
		{
			auto driveEnabled = getRegisterValue(Registry::RegisterType::ControlMode) > 0;
			gpio_set_level(MM_CONFIG_MOTOR_DRIVER_PIN_ENABLE, driveEnabled ? 1 : 0);
		}
#endif
	}

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

#define MOD(a,b) ((((a)%(b))+(b))%(b))
	//----------
	void IRAM_ATTR
	MotorDriver::setTorque(Torque torque, PositionWithinStepCycle positionWithinStepCycle)
	{
		const auto driveOffset = getRegisterValue(Registry::RegisterType::DriveOffset);

		bool positiveDirection = torque >= 0;

		//presume position is 0 to start and we're moving positive

		int8_t coil_A;
		int8_t coil_B;

		if(driveOffset != 64) {
			// Full math version
			const auto offset = (int16_t) driveOffset; // 64 is 90 degree offset, i.e. one step
			if(positiveDirection) {
				coil_A = this->cosTableFull[MOD((int16_t) positionWithinStepCycle + offset, 255)];
				coil_B = this->cosTableFull[MOD((int16_t) positionWithinStepCycle - (int16_t)64 + offset, 255)];
			}
			else {
				coil_A = this->cosTableFull[MOD((int16_t) positionWithinStepCycle - offset, 255)];
				coil_B = this->cosTableFull[MOD((int16_t) positionWithinStepCycle - (int16_t)64 - offset, 255)];
			}
		}
		else {
			// NOTE : The overflow doesn't happen cleanly, we need to manually overflow
			// Quick version (unsigned fixed point math, avoid up/down casting)
			if(positiveDirection) {
				if(positionWithinStepCycle < 192) {
					coil_A = this->cosTableFull[positionWithinStepCycle + (uint8_t) 64];
				}
				else {
					coil_A = this->cosTableFull[positionWithinStepCycle - (uint8_t) 192];
				}
				coil_B = this->cosTableFull[positionWithinStepCycle];
			}
			else {
				if(positionWithinStepCycle < 64) {
					coil_A = this->cosTableFull[positionWithinStepCycle + (uint8_t) 192];
				}
				else {
					coil_A = this->cosTableFull[positionWithinStepCycle - (uint8_t) 64];
				}

				if(positionWithinStepCycle < 128) {
					coil_B = this->cosTableFull[positionWithinStepCycle + (uint8_t) 128];
				}
				else {
					coil_B = this->cosTableFull[positionWithinStepCycle - (uint8_t) 128];
				}
			}
		}
		

		

		// Coil A
		uint8_t referenceVoltageA = uint8_t (((uint16_t) abs(coil_A) * (uint16_t) abs(torque)) / (uint16_t) (64));
		dac_output_voltage(this->configuration.vrefDacs.A, referenceVoltageA);

		// Coil B
		uint8_t referenceVoltageB = uint8_t (((uint16_t) abs(coil_B) * (uint16_t) abs(torque)) / (uint16_t) (64));
		dac_output_voltage(this->configuration.vrefDacs.B, referenceVoltageB);

		gpio_set_level(this->configuration.coilPins.coil_A_positive, coil_A > 0);
		gpio_set_level(this->configuration.coilPins.coil_A_negative, coil_A < 0);
		gpio_set_level(this->configuration.coilPins.coil_B_positive, coil_B > 0);
		gpio_set_level(this->configuration.coilPins.coil_B_negative, coil_B < 0);
	}

	//----------
	void
	MotorDriver::step(uint8_t index, uint8_t current)
	{
#ifdef MM_CONFIG_MOTOR_DRIVER_ENABLE_ENABLED
		gpio_set_level(MM_CONFIG_MOTOR_DRIVER_PIN_ENABLE, 1);
#endif

		for(uint8_t i=0; i<4; i++) {
			gpio_set_level(this->configuration.coilPinArray[i], i == index);
		}

		for(uint8_t i=0; i<2; i++) {
			dac_output_voltage(this->configuration.vrefDacArray[i], current);
		}
	}
}
