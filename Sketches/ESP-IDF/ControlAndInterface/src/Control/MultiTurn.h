#pragma once
#include "EncoderCalibration.h"
#include "../DataTypes.h"

namespace Control {
	class MultiTurn {
	public:
		MultiTurn(EncoderCalibration &);
		void init(SingleTurnPosition);
		void update(SingleTurnPosition);
		MultiTurnPosition getMultiTurnPosition() const;
	private:
		EncoderCalibration & encoderCalibration;
		SingleTurnPosition priorSingleTurnPosition = 0;
		Turns turns = 0;
		MultiTurnPosition position = 0;
	};
}