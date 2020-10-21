#pragma once
#include "EncoderCalibration.h"
#include "../DataTypes.h"

#include "FreeRTOS.h"

namespace Control {
	class MultiTurn {
	public:
		struct SaveData {
			uint8_t fileIndex = 0;
			uint64_t saveSequenceIndex = 0;
			MultiTurnPosition multiTurnPosition;
			uint16_t checkSum;
		};

		MultiTurn(EncoderCalibration &);
		~MultiTurn();

		void init(SingleTurnPosition);
		void update(SingleTurnPosition);
		MultiTurnPosition getMultiTurnPosition() const;

		bool loadSession(SingleTurnPosition currentSingleTurn);
		SaveData lastSave;
	private:
		bool loadSessionFile(const char * filename, SaveData &);

		EncoderCalibration & encoderCalibration;
		SingleTurnPosition priorSingleTurnPosition = 0;
		Turns turns = 0;
		volatile MultiTurnPosition position = 0;
		TaskHandle_t saveTaskHandle;
	};
}