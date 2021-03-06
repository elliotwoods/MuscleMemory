#pragma once

#include "stdint.h"
#include "U8G2lib.h"

namespace Devices {
	class INA219
	{
	public:
		enum class Register
		{
			Configuration = 0x00,
			ShuntVoltage = 0x01,
			BusVoltage = 0x02,
			Power = 0x03,
			Current = 0x04,
			Calibration = 0x05
		};

		struct Configuration
		{
			Configuration() {
				
			}

			enum VoltageRange : uint8_t
			{
				From_0_to_16V = 0,
				From_0_to_32V = 1
			};

			enum Gain : uint8_t
			{
				Gain_1_Range_40mV = 0,
				Gain_2_Range_80mV = 1,
				Gain_4_Range_160mV = 2,
				Gain_8_Range_320mV = 3

				,
				Gain_Auto = 8
			};

			enum ADCResolution : uint8_t
			{
				Resolution9bit_84us = 0,
				Resolution10bit_148us = 1,
				Resolution11bit_276us = 2,
				Resolution12bit_532us = 3,
				Resolution12bit_532us_2 = 8

				,
				Samples2_1_06ms = 9 // 1.06ms, etc
				,
				Samples4_2_13ms = 10,
				Samples8_4_26ms = 11,
				Samples16_8_51ms = 12,
				Samples32_17_02ms = 13,
				Samples64_34_05ms = 14,
				Samples128_68_10ms = 15
			};

			enum OperatingMode : uint8_t
			{
				PowerDown = 0,
				ShuntVoltageTriggered = 1,
				BusVoltageTriggered = 2,
				ShuntAndBusTriggered = 3

				,
				ADCOff = 4

				,
				ShuntVoltageContinuous = 5,
				BusVoltageContinuous = 6,
				ShuntAndBusContinuous = 7
			};

			uint8_t address = 0b1000000;

			VoltageRange voltageRange = VoltageRange::From_0_to_32V;
			Gain gain = Gain::Gain_Auto;
			ADCResolution currentResolution = ADCResolution::Samples128_68_10ms;
			ADCResolution busVoltageResolution = ADCResolution::Samples128_68_10ms;
			OperatingMode operatingMode = OperatingMode::ShuntAndBusContinuous;

			// Shunt resistor value in Ohms
			float shuntValue = 5e-3;

			// Maximum expected current in Amps. This is used to calculate the current LSB readback and gain values
			// Note if this value is too small, then we get an overflow in the calibration register (e.g. 2A is too low with a 5mOhm shunt)
			float maximumCurrent = 8;
		};

		enum Errors : uint8_t
		{
			Overflow,
			NoAcknowledge
		};

		INA219();
		void init(const Configuration & = Configuration());

		float getCurrent();
		float getPower();
		float getBusVoltage();
		float getShuntVoltage();

		void printDebug();
		void drawDebug(U8G2 &oled);

	private:
		uint16_t readRegister(Register);
		void writeRegister(Register, uint16_t value);

		void reset();
		void setConfiguration();
		void setCalibration();

		void calculateGain();
		Configuration configuration;

		uint8_t errors = 0;

		// Cached inside setCalibration() method
		float currentLSB;
	};

}
