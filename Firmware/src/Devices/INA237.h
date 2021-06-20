#pragma once

#include "stdint.h"
#include "U8G2lib.h"

namespace Devices {
	class INA237
	{
	public:
		enum class Register
		{
			Configuration = 0x0,
			ADCConfiguration = 0x2,
			ShuntCalibration = 0x3,
			
			ShuntVoltage = 0x4,
			BusVoltage = 0x5,
			Temperature = 0x6,
			Current = 0x7,
			Power = 0x8,

			DiagnosticFlags = 0xB,
			ShuntOvervoltageThreshold = 0xC,
			ShuntUndervoltageThreshold = 0xD,
			BusOvervoltageThreshold = 0xE,
			BusUndervoltageThreshold = 0xF,
			TemperatureLimit = 0x10,
			PowerLimit = 0x11,

			ManufacturerID = 0x3E,
			DeviceID = 0x3F
		};

		struct Configuration
		{
			Configuration() {
				
			}
			
			enum ConversionDelay : uint8_t
			{
				ConversionDelay_0s = 0x0,
				ConversionDelay_2ms = 0x1,
				ConversionDelay_510ms = 0xFF
			};

			enum ShuntRange : uint8_t
			{
				ShuntRange_163_84mV = 0,
				ShuntRange_40_96mV = 1
			};

			enum OperatingMode : uint8_t
			{
				Shutdown = 0x0,
				Triggered_BusVoltage = 0x1,
				Triggered_ShuntVoltage = 0x2,
				Triggered_ShuntAndBusVoltage = 0x3,
				Triggered_Temperature = 0x4,
				Triggered_TemperatureAndBusVoltage = 0x5,
				Triggered_TemperatureAndShuntVoltage = 0x6,
				Triggered_TemperatureAndBusAndShuntVoltage = 0x7,

				Shutdown2 = 0x8,

				Continuous_BusVoltage = 0x9,
				Continuous_ShuntVoltage = 0xA,
				Continuous_ShuntAndBusVoltage = 0xB,
				Continuous_Temperature = 0xC,
				Continuous_TemperatureAndBusVoltage = 0xD,
				Continuous_TemperatureAndShuntVoltage = 0xE,
				Continuous_TemperatureAndBusAndShuntVoltage = 0xF
			};

			enum ConversionTime : uint8_t
			{
				Conversion_50us = 0,
				Conversion_84us = 1,
				Conversion_150us = 2,
				Conversion_280us = 3,
				Conversion_540us = 4,
				Conversion_1052us = 5,
				Conversion_2074us = 6,
				Conversion_4120us = 7
			};

			enum SampleCount : uint8_t
			{
				SampleCount_1 = 0,
				SampleCount_4 = 1,
				SampleCount_16 = 2,
				SampleCount_64 = 3,
				SampleCount_128 = 4,
				SampleCount_256 = 5,
				SampleCount_512 = 6,
				SampleCount_1024 = 7
			};

			uint8_t address = 0b1000000;

			ConversionDelay conversionDelay = ConversionDelay::ConversionDelay_0s;
			ShuntRange shuntRange = ShuntRange::ShuntRange_163_84mV;
			OperatingMode operatingMode = OperatingMode::Continuous_TemperatureAndBusAndShuntVoltage;
			ConversionTime busVoltageConversionTime = ConversionTime::Conversion_150us;
			ConversionTime shuntVoltageConversionTime = ConversionTime::Conversion_150us;
			ConversionTime temperatureConversionTime = ConversionTime::Conversion_150us;
			SampleCount sampleCount = SampleCount::SampleCount_64;

			// Shunt resistor value in Ohms (2 * 33mOhm in parallel for MMv3)
			float shuntValue = 16.5e-3;

			// Used to set the LSB in setShuntCalibration
			float maximumCurrent = 8;
		};

		enum DiagnosticFlags : uint16_t
		{
			AlertLatchEnable = 1 << 15,
			ConversionReady = 1 << 13,
			AlertOnAveragedValue = 1 << 13,
			AlertPolarity = 1 << 12,

			OverflowError = 1 << 9,
			
			TemperatureHighAlert = 1 << 7,
			ShuntVoltageHighAlert = 1 << 6,
			ShuntVoltageLowAlert = 1 << 5,
			BusVoltageHighAlert = 1 << 4,
			BusVoltageLowAlert = 1 << 3,
			PowerHighAlert = 1 << 2,
			ConversionCompleted = 1 << 1,
			MemoryChecksumError = 1 << 0
		};

		INA237();
		void init(const Configuration & = Configuration());

		float getCurrent();
		float getPower();
		float getBusVoltage();
		float getShuntVoltage();
		float getTemperature();

		void printDebug();
		void drawDebug(U8G2 &oled);

	private:
		uint16_t readRegister(Register);
		void writeRegister(Register, uint16_t value);

		void reset();
		void setConfiguration();
		void setADCConfiguration();
		void setShuntCalibration();

		Configuration configuration;

		uint16_t errors = 0;

		// Cached inside setShuntCalibration() method
		float currentLSB;
	};
}