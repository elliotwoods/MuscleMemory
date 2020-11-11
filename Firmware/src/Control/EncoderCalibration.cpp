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
		this->hasCalibration = false;
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
		loadItem(file, this->stepCycleCalibration.stepCycleOffset);
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
		
		this->hasCalibration = true;
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
		saveItem(file, this->stepCycleCalibration.stepCycleOffset);
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
	bool
	EncoderCalibration::getHasCalibration() const
	{
		return this->hasCalibration;
	}

	//----------
	const EncoderCalibration::StepCycleCalibration &
	EncoderCalibration::getStepCycleCalibration() const
	{
		return this->stepCycleCalibration;
	}

	//----------
	bool
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
		
				u8g2.drawStr(5, 11, guiStatus);

				char message[100];

				{
					sprintf(message, "Enc AGC %.2f %s%s%s%s"
						, (float) guiInfo.encoderDiagnostics.automaticGainControl / ((float) (1 << 16))
						, guiInfo.encoderDiagnostics.fieldStrengthTooHigh ? "HIGH " : ""
						, guiInfo.encoderDiagnostics.fieldStrengthTooLow ? "LOW " : ""
						, guiInfo.encoderDiagnostics.cordicOverflow ? "OVER " : ""
						, guiInfo.encoderDiagnostics.internalOffsetLoopFinished ? "FIN " : ""
					);
					u8g2.drawStr(5, 34, message);
				}

				{
					sprintf(message, "Enc Value : %d (%.2f)"
						, guiInfo.encoderReading
						, guiInfo.encoderReadingNormalised * this->settings.stepsPerRevolution
					);
					u8g2.drawStr(5, 44, message);
				}

				{
					sprintf(message, "Enc cordic mag : %d (%.2f%%)"
						, guiInfo.encoderCordicMagnitude
						, guiInfo.encoderCordicMagnitudeNormalised * 100.0f
					);
					u8g2.drawStr(5, 54, message);
				}

				u8g2.drawFrame(0, 0, 128, 64);
			};
		}

		try {
			// Step up to the lowest encoder value
			uint16_t stepIndexOffset;
			{
				const auto encoderTicksPerStep = (1 << 14) / this->settings.stepsPerRevolution;

				uint8_t step = 0;

				// Perform first step
				motorDriver.step(step, settings.current);
				vTaskDelay(settings.stepHoldTimeMS / portTICK_PERIOD_MS);
				auto startValue = encoder.getPositionFiltered(settings.encoderFilterSize);

				//check if we're already at the start
				if(startValue < encoderTicksPerStep) {
					printf("Already at first step (start value = %d)\n", startValue);
				}
				else {
					printf("Stepping to first step in encoder range (start value = %d)...\n", startValue);
					sprintf(guiStatus, "Step to start");
					gui.update();

					// Step until we get to the beginning of the encoder readings
					auto position = startValue;
					do {
						motorDriver.step(++step % 4, settings.current);

						// Move fast until we get close to the end
						auto period = position > (1 << 14) * (stepCycleCount - 1) / stepCycleCount
							? settings.stepHoldTimeMS
							: settings.stepToStartPeriodMS;

						vTaskDelay(period / portTICK_PERIOD_MS);
						position = encoder.getPositionFiltered(settings.encoderFilterSize);
						gui.update();
					} while (position > startValue);
				}

				stepIndexOffset = step % 4;

				printf("Encoder is now : %d , stepIndexOffset : %d \n", encoder.getPositionFiltered(settings.encoderFilterSize), stepIndexOffset);
				gui.update();
			}

			vTaskDelay(settings.pauseTimeBetweenCyclesMS / portTICK_PERIOD_MS);
			
			// Record data
			{
				EncoderReading priorPosition = encoder.getPositionFiltered(this->settings.encoderFilterSize);

				uint16_t step = 0;
				uint16_t priorStep = 0; // This is true for first step
				
				// alternate forwards and backwards
				for(uint8_t cycle = 0; cycle < settings.cycles; cycle++) {
					printf("Calibration cycle : %d \n", cycle);

					if(cycle % 2 == 0) {
						// step forwards
						for(; ; step++) {
							sprintf(guiStatus, "Cycle : %d, Step : %d\n", cycle, step);
							this->recordStep(step
								, priorStep
								, stepIndexOffset
								, encoder
								, motorDriver
								, accumulatedEncoderValue
								, visitsPerStepCycle
								, priorPosition);
							priorStep = step;
							gui.update();

							// For some reason this is corrupted on the gui update
							priorPosition = encoder.getPositionFiltered(this->settings.encoderFilterSize);

							// The next cycle should start here, not at next step
							if(step == settings.stepsPerRevolution - 1) {
								break;
							}
						}
					}
					else {
						// step backwards
						for(; ; step--) {
							sprintf(guiStatus, "Cycle : %d, Step : %d\n", cycle, step);
							this->recordStep(step
								, priorStep
								, stepIndexOffset
								, encoder
								, motorDriver
								, accumulatedEncoderValue
								, visitsPerStepCycle
								, priorPosition);
							priorStep = step;
							gui.update();

							// For some reason this is corrupted on the gui update
							priorPosition = encoder.getPositionFiltered(this->settings.encoderFilterSize);

							// Don't underflow the step, also next cycle should start here
							if(step == 0) {
								break;
							}
						}
					}

					gui.update();
					vTaskDelay(settings.pauseTimeBetweenCyclesMS / portTICK_PERIOD_MS);
				}
			}

			// Create the StepCycleCalibration (average the values)
			{
				sprintf(guiStatus, "Averaging values");
				this->stepCycleCalibration.stepCycleCount = stepCycleCount;
				this->stepCycleCalibration.encoderValuePerStepCycle = new uint16_t[stepCycleCount];
				for(uint16_t i=0; i<stepCycleCount; i++) {
					if(visitsPerStepCycle[i] == 0) {
						throw(Exception("Error : 0 visits for step index %d\n", i));
					}
					this->stepCycleCalibration.encoderValuePerStepCycle[i] = accumulatedEncoderValue[i] / visitsPerStepCycle[i];
				}
				this->stepCycleCalibration.encoderPerStepCycle = (1 << 14) / this->stepCycleCalibration.stepCycleCount; // gradient
			}

			// Store the stepCycleOffset
			this->stepCycleCalibration.stepCycleOffset = (uint8_t) stepIndexOffset * 64;

			// Save the calibration
			this->hasCalibration = true;
			if(!this->save()) {
				throw(Exception("Save failed"));
			}

			// Notify success
			printf("Calibration complete\n");
			panel->onDraw = [](U8G2 & u8g2) {
				u8g2.setFont(u8g2_font_10x20_mr);
				u8g2.drawStr(0, 32, "CALIBRATE OK");
				u8g2.setFont(u8g2_font_nerhoe_tr);
			};

			gui.update();
			vTaskDelay(5000 / portTICK_PERIOD_MS);
		}
		catch(const Exception & e) {
			this->hasCalibration = false;

			printf("CALIBRATION FAILED : %s\n", e.what());

			panel->onDraw = [e](U8G2 & u8g2) {
				u8g2.setFont(u8g2_font_10x20_mr);
				u8g2.drawStr(0, 16, "CALIBRATION");
				u8g2.drawStr(0, 32, "FAIL!");

				u8g2.setFont(u8g2_font_nerhoe_tr);
				u8g2.drawStr(5, 50, e.what());

				u8g2.setDrawColor(2);
				u8g2.drawBox(0, 0, 128, 64);
				u8g2.setDrawColor(1);
			};

			bool buttonPressed = false;
			panel->onButtonPressed = [&buttonPressed] {
				buttonPressed = true;
			};
			auto startTime = esp_timer_get_time();
			while(!buttonPressed && esp_timer_get_time() - startTime < 10 * 1000000) {
				gui.update();
				vTaskDelay(50 / portTICK_PERIOD_MS);
			}
		}

		// Clean up data
		delete[] accumulatedEncoderValue;
		delete[] visitsPerStepCycle;

		// remove our preview panel
		gui.popPanel();

		// Remember if calibration completed, and exit returning this information
		return this->hasCalibration;
	}

	//----------
	PositionWithinStepCycle
	EncoderCalibration::getPositionWithinStepCycle(EncoderReading encoderReading) const
	{
		if(!this->hasCalibration) {
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
				) + this->stepCycleCalibration.stepCycleOffset;
			}
		}
		return 0;
	}

	//----------
	void
	EncoderCalibration::recordStep(uint16_t stepIndex
		, uint16_t priorStep
		, uint16_t stepIndexOffset
		, Devices::AS5047 & encoder
		, Devices::MotorDriver & motorDriver
		, uint32_t * accumulatedEncoderValue
		, uint8_t * visitsPerStepCycle
		, const EncoderReading & priorPosition)
	{
		// Size of one step in encoder readings
		const int32_t stepMagnitude = (1 << 14) / (int) this->settings.stepsPerRevolution;

		// Perform the step
		printf("Step : %d, Offset : %d\n", stepIndex, stepIndexOffset);
		motorDriver.step((stepIndex + stepIndexOffset) % 4, this->settings.current);
		vTaskDelay(this->settings.stepHoldTimeMS / portTICK_PERIOD_MS);

		// Take the reading
		auto position = (uint32_t) encoder.getPositionFiltered(settings.encoderFilterSize);
		printf("Position : %d, Prior : %d\n", position, priorPosition);

		// check skipped steps by magnitude
		if(stepIndex != priorStep) {
			const auto delta = (int32_t) abs((int32_t)position - (int32_t)priorPosition);
			if(delta < stepMagnitude / 2) {
				throw(Exception("Step %d underflow (%d/%d)", stepIndex, delta, stepMagnitude));
			}
			else if(delta > stepMagnitude * 3 / 2) {
				throw(Exception("Step %d overflow (%d/%d)", stepIndex, delta, stepMagnitude));
			}
		}

		// Only record complete step cycles
		if(stepIndex % 4 == 0) {
			if(stepIndex < this->settings.stepsPerRevolution * 1 / 4 && position > (1 << 14) * 3 / 4) {
				// We've underflowed the encoder - don't record this sample
				return;
			}
			if(stepIndex > this->settings.stepsPerRevolution * 3 / 4 && position < (1 << 14) * 1 / 4) {
				// We've overflowed the encoder - offset the sample
				position += 1 << 14;
			}
			printf("stepIndex (%d), encoder reading (%d)\n"
				, stepIndex
				, position);

			auto stepCycle = stepIndex / 4;

			// Check skipped steps by change from prior readings
			if(visitsPerStepCycle[stepCycle] > 0) {
				const auto priorMean = accumulatedEncoderValue[stepCycle] / visitsPerStepCycle[stepCycle];
				const auto delta = (int32_t) abs(priorMean - position);
				const auto allowableDeviation = 8;
				if(delta > allowableDeviation) {
					throw(Exception("Deviation over (%d/%d) at %d", delta, allowableDeviation, stepIndex));
				}
			}

			accumulatedEncoderValue[stepCycle] += position;
			visitsPerStepCycle[stepCycle]++;
		}
	}
}