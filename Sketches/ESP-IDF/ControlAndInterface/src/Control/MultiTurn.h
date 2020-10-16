#pragma once
#include "EncoderCalibration.h"
#include "../Devices/AS5047.h"
#include "../DataTypes.h"

namespace Control {
	class MultiTurn {
	public:
		MultiTurn(Devices::AS5047 &, EncoderCalibration &);
		void init();
		void update(PositionWithinShaftCycle);
		MultiTurnPosition getMultiTurnPosition() const;
	private:
		Devices::AS5047 & as5047;
		EncoderCalibration & encoderCalibration;
		PositionWithinShaftCycle priorSingleTurnPosition = 0;
		Turns turns = 0;
		MultiTurnPosition position = 0;
	};
}