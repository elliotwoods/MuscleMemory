#pragma once

#include "AS5047.h"
#include "MotorDriver.h"

class EncoderCalibration {
public:
	struct Settings {
		Settings() { }
		uint16_t stepsPerRevolution = 360 / 1.8;
		uint8_t current = 64;
		uint8_t cycles = 8;

		/// The amount of time to hold the step for before taking a reading
		/// Note : this also affects the speed of the calibration routine
		uint16_t stepHoldTimeMS = 50;
		uint16_t pauseTimeBetweenCyclesMS = 500;
	};

	void calibrate(AS5047 &
		, MotorDriver &
		, const Settings & settings = Settings());
private:
	void recordStep(uint16_t stepIndex
		, AS5047 & encoder
		, MotorDriver & motorDriver
		, uint32_t * accumulatedEncoderValue
		, uint8_t * visitsPerStep);
	Settings settings;
};