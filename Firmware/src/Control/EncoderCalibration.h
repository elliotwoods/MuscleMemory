#pragma once

#include "../Devices/AS5047.h"
#include "../Devices/MotorDriver.h"

#include "DataTypes.h"
#include "Exception.h"

#include "stdint.h"

#include "GUI/Panel.h"

namespace Control {
	class EncoderCalibration {
	public:
		struct StepCycleCalibration {
			uint8_t stepCycleOffset = 0; // Offset of step cycle vs encoder (in full steps)
			uint16_t * encoderValuePerStepCycle = nullptr;
			uint16_t stepCycleCount;
			uint16_t encoderPerStepCycle;
		};

		struct Settings {
			Settings() { }
			uint8_t encoderFilterSize = 255;
			uint16_t stepsPerRevolution = 360 / 1.8;
			uint8_t current = 40; // Equivalent amperes value 
			uint8_t cycles = 2; // Iterations to run the calibration routine for

			/// The amount of time to hold the step for before taking a reading
			/// Note : this also affects the speed of the calibration routine
			uint16_t stepHoldTimeMS = 100;

			uint16_t stepToStartPeriodMS = 1;

			uint16_t pauseTimeBetweenCyclesMS = 500;
		};

		~EncoderCalibration();
		
		void clear();
		void load(FILE *);
		void save(FILE *);
		bool load();
		bool save();

		bool getHasCalibration() const;
		const StepCycleCalibration & getStepCycleCalibration() const;

		bool calibrate(Devices::AS5047 &
			, Devices::MotorDriver &
			, const Settings & settings = Settings());

		PositionWithinStepCycle getPositionWithinStepCycle(EncoderReading) const;
	private:
		void recordStep(uint16_t stepIndex
			, uint16_t priorStep
			, uint16_t stepIndexOffset
			, Devices::AS5047 & encoder
			, Devices::MotorDriver & motorDriver
			, uint32_t * accumulatedEncoderValue
			, uint8_t * visitsPerStep
			, const EncoderReading & priorPosition);
		Settings settings;
		StepCycleCalibration stepCycleCalibration;

		bool hasCalibration = false;
	};
}