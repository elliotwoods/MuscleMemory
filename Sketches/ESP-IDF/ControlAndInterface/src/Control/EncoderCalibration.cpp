#include "EncoderCalibration.h"
#include "stdio.h"
#include "GUI/Controller.h"
#include "Registry.h"
#include "GUI/Panels/Lambda.h"

const char filePath[] = "/appdata/encoder.dat";

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
	loadItem(FILE * file, T & data)
	{
		fread(&data, sizeof(T), 1, file);
	}

	template<typename T>
	void
	loadArray(FILE * file, T *& data)
	{
		if(data) {
			delete[] data;
			data = nullptr;
		}

		size_t size;
		loadItem(file, size);

		data = new T[size];
		fread(data, sizeof(T), size, file);
	}

	void
	EncoderCalibration::load(FILE * file)
	{
		loadItem(file, this->stepCycleCalibration.stepCycleCount);
		loadItem(file, this->stepCycleCalibration.encoderPerStepCycle);
		loadArray(file, this->stepCycleCalibration.encoderValuePerStepCycle);
	}

	//----------
	bool
	EncoderCalibration::load()
	{
		auto fileExists = access(filePath, F_OK) != -1;
		if(fileExists) {
			FILE * file;
			if((file = fopen(filePath, "rb"))) {
				load(file);
				fclose(file);
			}
			else {
				printf("Failed to open file for reading : %s\n", filePath);
				return false;
			}
		}
		else {
			printf("Cannot load EncoderCalibration : the file %s does not exist\n", filePath);
			return false;
		}
		
		return true;
	}

	//----------
	template<typename T>
	void
	saveItem(FILE * file, const T & data)
	{
		fwrite(&data, sizeof(T), 1, file);
	}

	template<typename T>
	void
	saveArray(FILE * file, const T * data, size_t size)
	{
		saveItem(file, size);
		fwrite(data, sizeof(T), size, file);
	}

	void
	EncoderCalibration::save(FILE * file)
	{
		saveItem(file, this->stepCycleCalibration.stepCycleCount);
		saveItem(file, this->stepCycleCalibration.encoderPerStepCycle);
		saveArray(file, this->stepCycleCalibration.encoderValuePerStepCycle, this->stepCycleCalibration.stepCycleCount);
	}

	//----------
	bool
	EncoderCalibration::save()
	{
		FILE * file;
		if((file = fopen(filePath, "wb"))) {
			save(file);
			fclose(file);
			return true;
		}
		else {
			printf("Failed to open file for writing : %s\n", filePath);
			return false;
		}
	}


	//----------
	void
	EncoderCalibration::calibrate(Devices::AS5047 & encoder
		, Devices::MotorDriver & motorDriver
		, const Settings & settings)
	{
		// Clear any existing calibration and cache the settings we used
		this->clear();
		this->settings = settings;

		// Allocate and clear the data storage
		auto stepCycleCount = settings.stepsPerRevolution / 4;
		auto accumulatedEncoderValue = new uint32_t[stepCycleCount];
		auto visitsPerStepCycle = new uint8_t[stepCycleCount];
		{
			memset(accumulatedEncoderValue, 0, sizeof(uint32_t) * stepCycleCount);
			memset(visitsPerStepCycle, 0, sizeof(uint8_t) * stepCycleCount);
		}
	
		// Initialise the GUI panel
		auto & gui = GUI::Controller::X();
		auto panel = std::make_shared<GUI::Panels::Lambda>();
		char guiStatus[100];
		sprintf(guiStatus, "Starting calibration...");
		gui.pushPanel(panel);
		{
			struct { 
				Devices::AS5047::Diagnostics encoderDiagnostics;
				EncoderReading encoderReading;
				float encoderReadingNormalised;
				uint16_t encoderCordicMagnitude;
				float encoderCordicMagnitudeNormalised;
			} guiInfo;
			panel->onUpdate = [&]() {
				guiInfo.encoderDiagnostics = encoder.getDiagnostics();
				guiInfo.encoderReading = encoder.getPositionFiltered(settings.encoderFilterSize);
				guiInfo.encoderReadingNormalised = (float) guiInfo.encoderReading / (float) (1 << 14);
				guiInfo.encoderCordicMagnitude = encoder.getCordicMagnitude();
				guiInfo.encoderCordicMagnitudeNormalised = guiInfo.encoderCordicMagnitude / (float) (1 << 14);
			};
			panel->onDraw = [&](U8G2 & u8g2) {
				u8g2.setFont(u8g2_font_nerhoe_tr);
		
				u8g2.drawStr(10, 10, guiStatus);

				char message[100];

				{
					sprintf(message, "Enc AGC %.2f %s%s%s%s"
						, (float) guiInfo.encoderDiagnostics.automaticGainControl / ((float) (1 << 16))
						, guiInfo.encoderDiagnostics.fieldStrengthTooHigh ? "HIGH " : ""
						, guiInfo.encoderDiagnostics.fieldStrengthTooLow ? "LOW " : ""
						, guiInfo.encoderDiagnostics.cordicOverflow ? "OVER " : ""
						, guiInfo.encoderDiagnostics.internalOffsetLoopFinished ? "FIN " : ""
					);
					u8g2.drawStr(10, 34, message);
				}

				{
					sprintf(message, "Enc Value : %d (%.2f%)"
						, guiInfo.encoderReading
						, guiInfo.encoderReadingNormalised * 100.0f
					);
					u8g2.drawStr(10, 44, message);
				}

				{
					sprintf(message, "Enc cordic mag : %d (%.2f%)"
						, guiInfo.encoderCordicMagnitude
						, guiInfo.encoderCordicMagnitudeNormalised * 100.0f
					);
					u8g2.drawStr(10, 54, message);
				}

				u8g2.drawFrame(0, 0, 128, 64);
			};
		}

		// Step up to the lowest encoder value
		{

			uint8_t step = 0;
			motorDriver.step(step, settings.current);
			auto startValue = encoder.getPositionFiltered(settings.encoderFilterSize);

			printf("Stepping to lowest encoder value (start value = %d)...\n", startValue);
			sprintf(guiStatus, "Step to start");
			gui.update();

			// Step until we get to the beginning of the encoder readings
			do {
				motorDriver.step(++step % 4, settings.current);
				vTaskDelay(settings.stepHoldTimeMS / portTICK_PERIOD_MS);
				gui.update();
			} while (encoder.getPositionFiltered(settings.encoderFilterSize) > startValue);

			// Step up to the start of the next step cycle
			while(step % 4 != 0) {
				motorDriver.step(step++ % 4, settings.current);
				vTaskDelay(settings.stepHoldTimeMS / portTICK_PERIOD_MS);
				gui.update();
			}

			printf("Encoder is now : %d , First step : %d \n", encoder.getPositionFiltered(settings.encoderFilterSize), step);
			gui.update();
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
						sprintf(guiStatus, "Cycle : %d, Step : %d\n", cycle, step);
						gui.update();
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
						sprintf(guiStatus, "Cycle : %d, Step : %d\n", cycle, step);
						gui.update();
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

				gui.update();
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
			sprintf(guiStatus, "Averaging values");
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

		// remove our preview panel
		gui.popPanel();
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
			auto position = encoder.getPositionFiltered(settings.encoderFilterSize);
		
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
			printf("stepIndex (%d), encoder reading (%d)\n"
				, stepIndex
				, position);

			auto stepCycle = stepIndex / 4;
			accumulatedEncoderValue[stepCycle] += position;
			visitsPerStepCycle[stepCycle]++;
		}
	}
}