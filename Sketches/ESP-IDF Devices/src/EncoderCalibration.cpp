#include "EncoderCalibration.h"

//----------
EncoderCalibration::~EncoderCalibration()
{
	this->clear();
}

//----------
void
EncoderCalibration::clear()
{
	if(this->stepCycleCalibration.encoderValuePerStepCycle) {
		delete[] this->stepCycleCalibration.encoderValuePerStepCycle;
	}
}

//----------
void
EncoderCalibration::calibrate(AS5047 & encoder
	, MotorDriver & motorDriver
	, const Settings & settings)
{
	this->clear();
	this->settings = settings;

	// Step up to the lowest encoder value
	{

		uint8_t step = 0;
		motorDriver.step(step++, settings.current);
		auto startValue = encoder.getPosition();

		//printf("Stepping to lowest encoder value (start value = %d)...\n", startValue);

		do {
			motorDriver.step(step++ % 4, settings.current);
			vTaskDelay(settings.stepHoldTimeMS / portTICK_PERIOD_MS);
		} while (encoder.getPosition() > startValue);

		//printf("Encoder is now : %d \n", encoder.getPosition());
	}

	vTaskDelay(settings.pauseTimeBetweenCyclesMS / portTICK_PERIOD_MS);

	
	// Record data
	{
		auto accumulatedEncoderValue = new uint32_t[settings.stepsPerRevolution];
		auto visitsPerStep = new uint8_t[settings.stepsPerRevolution];

		memset(accumulatedEncoderValue, 0, sizeof(uint32_t) * settings.stepsPerRevolution);
		memset(visitsPerStep, 0, sizeof(uint8_t) * settings.stepsPerRevolution);

		// alternate forwards and backwards
		for(uint8_t cycle = 0; cycle < settings.cycles; cycle++) {
			printf("Calibration cycle : %d \n", cycle);

			if(cycle % 2 == 0) {
				// step forwards
				for(uint16_t step = 0; step < settings.stepsPerRevolution; step++) {
					this->recordStep(step
						, encoder
						, motorDriver
						, accumulatedEncoderValue
						, visitsPerStep);
				}
			}
			else {
				// step backwards
				for(uint16_t step = settings.stepsPerRevolution - 1; step > 0; step--) {
					this->recordStep(step
						, encoder
						, motorDriver
						, accumulatedEncoderValue
						, visitsPerStep);
				}
			}

			vTaskDelay(settings.pauseTimeBetweenCyclesMS / portTICK_PERIOD_MS);
		}

		//printf("\n");


		// Take the averaged value
		auto averagedValue = new uint16_t[settings.stepsPerRevolution];
		for(uint16_t i=0; i<settings.stepsPerRevolution; i++) {
			if(i > 0) {
				//printf(", ");
			}
			
			averagedValue[i] = accumulatedEncoderValue[i] / visitsPerStep[i];
			//printf("%d", averagedValue[i]);
		}
		//printf("\n");

		// Create the StepCycleCalibration
		{
			this->stepCycleCalibration.stepCycleCount = settings.stepsPerRevolution / 4;
			this->stepCycleCalibration.encoderValuePerStepCycle = new uint16_t[this->stepCycleCalibration.stepCycleCount];
			for(uint16_t i=0; i<settings.stepsPerRevolution; i+= 4) {
				this->stepCycleCalibration.encoderValuePerStepCycle[i / 4] = averagedValue[i];
			}
			this->stepCycleCalibration.encoderPerStepCycle = (1 << 14) / this->stepCycleCalibration.stepCycleCount; // gradient
		}

		delete[] averagedValue;
		delete[] accumulatedEncoderValue;
		delete[] visitsPerStep;
	}
}

//----------
PositionWithinStepCycle
EncoderCalibration::getPositionWithinStepCycle(EncoderReading encoderReading) const
{
	if(!this->stepCycleCalibration.encoderValuePerStepCycle) {
#ifndef NDEBUG
		printf("No calibration\n");
#endif
		return 0;
	}

	if(encoderReading < this->stepCycleCalibration.encoderValuePerStepCycle[0]) {
		// we're before the first recorded step, so cycle it around to the end
		encoderReading += 1 << 14;
	}

	for(size_t i=0; i<this->stepCycleCalibration.stepCycleCount; i++) {
		auto encoderReadingAtStartOfStepCycle = this->stepCycleCalibration.encoderValuePerStepCycle[i];

		if(i + 1 >= this->stepCycleCalibration.stepCycleCount
		|| (
			encoderReading >= encoderReadingAtStartOfStepCycle
			&& encoderReading < this->stepCycleCalibration.encoderValuePerStepCycle[i + 1]
		)) {
			auto encoderReadingWithinCycle = encoderReading - encoderReadingAtStartOfStepCycle;
			return (uint8_t)(
				(uint32_t) encoderReadingWithinCycle
				 * (uint32_t) 255
				 / this->stepCycleCalibration.encoderPerStepCycle
			);
		}
	}
	return 0;
}

//----------
void
EncoderCalibration::recordStep(uint16_t stepIndex
	, AS5047 & encoder
	, MotorDriver & motorDriver
	, uint32_t * accumulatedEncoderValue
	, uint8_t * visitsPerStep)
{
	motorDriver.step(stepIndex % 4, this->settings.current);

	vTaskDelay(this->settings.stepHoldTimeMS / portTICK_PERIOD_MS);

	auto position = encoder.getPosition();

	if(stepIndex == 0 && position > 1 << 13) {
		// Ee've underflowed the encoder - don't record this sample
		return;
	}
	if(stepIndex == this->settings.stepsPerRevolution - 1 && position < 1 << 13)
	{
		// We've overflowed the encoder - offset the sample
		position += 1 << 14;
	}

	accumulatedEncoderValue[stepIndex]+= position;
	visitsPerStep[stepIndex]++;
}