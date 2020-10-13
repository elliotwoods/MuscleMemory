#include "EncoderCalibration.h"
#include <EEPROM.h>

namespace Control {
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
			this->stepCycleCalibration.encoderValuePerStepCycle = nullptr;
		}
	}

	//----------
	template<typename T>
	void
	loadItem(uint16_t & position, T & data)
	{
		auto rawData = (uint8_t*) & data;
		for(uint16_t i=0; i<sizeof(data); i++) {
			*rawData++ = EEPROM.read(position++);
		}
	}

	template<typename T>
	void
	loadArray(uint16_t & position, T *& data)
	{
		if(data) {
			delete[] data;
			data = nullptr;
		}

		size_t size;
		loadItem(position, size);

		data = new T[size];
		for(size_t i=0; i<size; i++) {
			loadItem(position, data[i]);
		}
	}

	void
	EncoderCalibration::load(uint16_t & position)
	{
		loadItem(position, this->stepCycleCalibration.stepCycleCount);
		loadItem(position, this->stepCycleCalibration.encoderPerStepCycle);
		loadArray(position, this->stepCycleCalibration.encoderValuePerStepCycle);
	}


	//----------
	template<typename T>
	void
	saveItem(uint16_t & position, const T & data)
	{
		auto rawData = (uint8_t*) & data;
		for(uint16_t i=0; i<sizeof(data); i++) {
			EEPROM.write(position++, *rawData++);
		}
	}

	template<typename T>
	void
	saveArray(uint16_t & position, const T * data, size_t size)
	{
		saveItem(position, size);
		for(size_t i=0; i<size; i++) {
			saveItem(position, data[i]);
		}
	}

	void
	EncoderCalibration::save(uint16_t & position)
	{
		saveItem(position, this->stepCycleCalibration.stepCycleCount);
		saveItem(position, this->stepCycleCalibration.encoderPerStepCycle);
		saveArray(position, this->stepCycleCalibration.encoderValuePerStepCycle, this->stepCycleCalibration.stepCycleCount);
	}

	//----------
	void
	EncoderCalibration::calibrate(Devices::AS5047 & encoder
		, Devices::MotorDriver & motorDriver
		, const Settings & settings)
	{
		this->clear();
		this->settings = settings;

		auto stepCycleCount = settings.stepsPerRevolution / 4;
		auto accumulatedEncoderValue = new uint32_t[stepCycleCount];
		auto visitsPerStepCycle = new uint8_t[stepCycleCount];

		memset(accumulatedEncoderValue, 0, sizeof(uint32_t) * stepCycleCount);
		memset(visitsPerStepCycle, 0, sizeof(uint8_t) * stepCycleCount);

		// Step up to the lowest encoder value
		{

			uint8_t step = 0;
			motorDriver.step(step, settings.current);
			auto startValue = encoder.getPosition();

			printf("Stepping to lowest encoder value (start value = %d)...\n", startValue);

			// Step until we get to the beginning of the encoder readings
			do {
				motorDriver.step(++step % 4, settings.current);
				vTaskDelay(settings.stepHoldTimeMS / portTICK_PERIOD_MS);
			} while (encoder.getPosition() > startValue);

			// Step up to the start of the next step cycle
			while(step % 4 != 0) {
				motorDriver.step(step++ % 4, settings.current);
				vTaskDelay(settings.stepHoldTimeMS / portTICK_PERIOD_MS);
			}

			printf("Encoder is now : %d , First step : %d \n", encoder.getPosition(), step);
		}

		vTaskDelay(settings.pauseTimeBetweenCyclesMS / portTICK_PERIOD_MS);
		
		// Record data
		{
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
							, visitsPerStepCycle);
						
					}
				}
				else {
					// step backwards
					for(uint16_t step = settings.stepsPerRevolution - 1; ; step--) {
						this->recordStep(step
							, encoder
							, motorDriver
							, accumulatedEncoderValue
							, visitsPerStepCycle);
						//printf(" BB step %d, visits %d \n", step, visitsPerStepCycle[step]);

						// Since the counter can't pass 0, we just manually break when we get to 0 on this one
						if(step == 0) {
							break;
						}
					}
				}

				vTaskDelay(settings.pauseTimeBetweenCyclesMS / portTICK_PERIOD_MS);
			}
			printf("------------- visits array----------- \n");
			for(int i = 0;i<sizeof(uint8_t) * stepCycleCount;i++){
				printf("%d - %d \n", i, visitsPerStepCycle[i]);
			}
			//printf("\n");
		}

		// Create the StepCycleCalibration (average the values)
		{
			this->stepCycleCalibration.stepCycleCount = stepCycleCount;
			this->stepCycleCalibration.encoderValuePerStepCycle = new uint16_t[stepCycleCount];
			for(uint16_t i=0; i<stepCycleCount; i++) {
				if(visitsPerStepCycle[i] == 0) {
					printf("Error : 0 visits for step index %d\n", i);
					abort();
				}
				this->stepCycleCalibration.encoderValuePerStepCycle[i] = accumulatedEncoderValue[i] / visitsPerStepCycle[i];
			}
			this->stepCycleCalibration.encoderPerStepCycle = (1 << 14) / this->stepCycleCalibration.stepCycleCount; // gradient
		}

		printf("Calibration complete\n");

		delete[] accumulatedEncoderValue;
		delete[] visitsPerStepCycle;
	}

	//----------
	PositionWithinStepCycle
	EncoderCalibration::getPositionWithinStepCycle(EncoderReading encoderReading) const
	{
		if(!this->stepCycleCalibration.encoderValuePerStepCycle) {
	#ifndef NDEBUG
			printf("No calibration\n");
			abort();
	#endif
			return 0;
		}

		if(encoderReading < this->stepCycleCalibration.encoderValuePerStepCycle[0]) {
			// we're before the first recorded step, so cycle it around to the end
			encoderReading += 1 << 14;
		}

		for(size_t i=0; i<this->stepCycleCalibration.stepCycleCount; i++) {
			auto encoderReadingAtStartOfStepCycle = this->stepCycleCalibration.encoderValuePerStepCycle[i];

			if(i + 1 >= this->stepCycleCalibration.stepCycleCount // we're at the last step cycle
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
		, Devices::AS5047 & encoder
		, Devices::MotorDriver & motorDriver
		, uint32_t * accumulatedEncoderValue
		, uint8_t * visitsPerStepCycle)
	{
		motorDriver.step(stepIndex % 4, this->settings.current);

		vTaskDelay(this->settings.stepHoldTimeMS / portTICK_PERIOD_MS);

		// Only record complete step cycles
		if(stepIndex % 4 == 0) {
			auto position = encoder.getPosition();
		
			if(stepIndex == 0 && position > 1 << 13) {
				// We've underflowed the encoder - don't record this sample. We recorded one sample when finding the first step
				return;
			}
			if(position> 1<<14){
				position -= 1 << 14;
			}
			if(stepIndex == this->settings.stepsPerRevolution - 1 && position < 1 << 13)
			{
				// We've overflowed the encoder - offset the sample
				position += 1 << 14;
			}

			auto stepCycle = stepIndex / 4;
			accumulatedEncoderValue[stepCycle] += position;
			visitsPerStepCycle[stepCycle]++;
		}
	}
}