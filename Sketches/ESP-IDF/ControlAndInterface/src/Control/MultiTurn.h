#pragma once
#include "EncoderCalibration.h"
#include "../DataTypes.h"

#include "FreeRTOS.h"

namespace Control {
	class MultiTurn {
	public:
		struct SaveData {
			uint8_t getCRC() const;

			uint8_t fileIndex = 0;
			uint64_t saveSequenceIndex = 0;
			MultiTurnPosition multiTurnPosition = 0;
			uint8_t storedCRC = 0;
		};

		MultiTurn(EncoderCalibration &);
		~MultiTurn();

		void init(SingleTurnPosition);
		void driveLoopUpdate(SingleTurnPosition);
		void mainLoopUpdate();
		MultiTurnPosition getMultiTurnPosition() const;

		void saveSession();
		bool loadSession(SingleTurnPosition currentSingleTurn);
	private:
		Turns implyTurns(MultiTurnPosition priorMultiTurnPosition, SingleTurnPosition currentSingleTurnPosition) const;
		static void renderFileName(char * filename, uint8_t index);
		static bool loadSessionFile(const char * filename, SaveData &);

		EncoderCalibration & encoderCalibration;
		SingleTurnPosition priorSingleTurnPosition = 0;
		Turns turns = 0;
		volatile MultiTurnPosition position = 0;
		
		SaveData saveData;
	};
}