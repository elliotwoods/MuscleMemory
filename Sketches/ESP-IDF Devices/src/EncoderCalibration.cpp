#include "EncoderCalibration.h"

//----------
void
EncoderCalibration::calibrate(AS5047 & encoder
	, MotorDriver & motorDriver
	, const Settings & settings)
{
	this->settings = settings;

	// Step up to the lowest encoder value
	{

		uint8_t step = 0;
		motorDriver.step(step++, settings.current);
		auto startValue = encoder.getPosition();

		printf("Stepping to lowest encoder value (start value = %d)...\n", startValue);

		do {
			motorDriver.step(step++ % 4, settings.current);
			vTaskDelay(settings.stepHoldTimeMS / portTICK_PERIOD_MS);
		} while (encoder.getPosition() > startValue);

		printf("Encoder is now : %d \n", encoder.getPosition());
	}
	
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
					motorDriver.step(step % 4, settings.current);

					vTaskDelay(settings.stepHoldTimeMS / portTICK_PERIOD_MS);

					accumulatedEncoderValue[step]+= encoder.getPosition();
					visitsPerStep[step]++;
				}
			}
			else {
				// step backwards
				for(uint16_t step = settings.stepsPerRevolution - 1; step > 0; step--) {
					motorDriver.step(step % 4, settings.current);

					vTaskDelay(settings.stepHoldTimeMS / portTICK_PERIOD_MS);

					accumulatedEncoderValue[step]+= encoder.getPosition();
					visitsPerStep[step]++;
				}
			}

			vTaskDelay(settings.pauseTimeBetweenCyclesMS / portTICK_PERIOD_MS);
		}

		for(uint16_t i=0; i<settings.stepsPerRevolution; i++) {
			if(i > 0) {
				printf(", ");
			}
			
			auto averagedValue = accumulatedEncoderValue[i] / visitsPerStep[i];
			printf("%d", averagedValue);
		}
		printf("\n");
		

		delete[] accumulatedEncoderValue;
		delete[] visitsPerStep;
	}

}