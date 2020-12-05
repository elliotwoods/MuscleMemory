#pragma once
#include "EncoderCalibration.h"
#include "../DataTypes.h"
#include "esp_partition.h"

namespace Control {
	class MultiTurn {
	public:
		struct SaveData {
			uint8_t getCRC() const;

			uint64_t saveSequenceIndex = 0;
			MultiTurnPosition multiTurnPosition = 0;
			uint8_t storedCRC = 0;
		};

		MultiTurn(EncoderCalibration &);
		~MultiTurn();

		void init(SingleTurnPosition);
		void driveLoopUpdate(SingleTurnPosition);
		void mainLoopUpdate();
		MultiTurnPosition getMultiTurnPositionNoOffset() const;
		MultiTurnPosition getMultiTurnPosition() const;

		void saveSession();
		bool loadSession(SingleTurnPosition currentSingleTurn);
	private:
		size_t getSaveDataSize() const;
		size_t getWriteOffsetForLastEntry() const;
		void formatPartition();
		Turns implyTurns(MultiTurnPosition priorMultiTurnPosition, SingleTurnPosition currentSingleTurnPosition) const;

		EncoderCalibration & encoderCalibration;
		SingleTurnPosition priorSingleTurnPosition = 0;
		Turns turns = 0;
		volatile MultiTurnPosition zeroOffset = 0;
		volatile MultiTurnPosition positionNoOffset = 0;
		volatile MultiTurnPosition position = 0;
		
		SaveData saveData;
		const esp_partition_t * partition;
		uint16_t saveIndex = 0;
	};
}