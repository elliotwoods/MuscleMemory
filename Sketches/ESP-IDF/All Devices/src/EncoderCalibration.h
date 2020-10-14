#pragma once

#include "AS5047.h"
#include "MotorDriver.h"
#include "DataTypes.h"

#include "stdint.h"

class EncoderCalibration {
public:
	struct Settings {
		Settings() { }
		uint16_t stepsPerRevolution = 360 / 1.8;
		uint8_t current = 64; // Equivalent amperes value 
		uint8_t cycles = 1; // Iterations to run the calibration routine for

		/// The amount of time to hold the step for before taking a reading
		/// Note : this also affects the speed of the calibration routine
		uint16_t stepHoldTimeMS = 50;

		uint16_t pauseTimeBetweenCyclesMS = 500;
	};

	~EncoderCalibration();
	void clear();
	void calibrate(AS5047 &
		, MotorDriver &
		, const Settings & settings = Settings());

	PositionWithinStepCycle getPositionWithinStepCycle(EncoderReading) const;
	uint16_t currentPosition(AS5047 & encoder);
private:
	struct {
		uint16_t * encoderValuePerStepCycle = nullptr;
		uint16_t stepCycleCount;
		uint16_t encoderPerStepCycle;
	} stepCycleCalibration;

	void recordStep(uint16_t stepIndex
		, AS5047 & encoder
		, MotorDriver & motorDriver
		, uint32_t * accumulatedEncoderValue
		, uint8_t * visitsPerStep);
	Settings settings;
};