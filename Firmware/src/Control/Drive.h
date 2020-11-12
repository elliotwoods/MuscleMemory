#pragma once

#include "MultiTurn.h"
#include "Agent.h"
#include "EncoderCalibration.h"

#include "Devices/MotorDriver.h"
#include "Devices/AS5047.h"

#include "Utils/FrameTimer.h"

#include "DataTypes.h"

namespace Control {
	class Drive {
	public:
		Drive(Devices::MotorDriver &
			, Devices::AS5047 &
			, EncoderCalibration &
			, MultiTurn &);

		void init();
		void update();
	private:
		Devices::MotorDriver & motorDriver;
		Devices::AS5047 & as5047;
		EncoderCalibration & encoderCalibration;
		MultiTurn & multiTurn;

		MultiTurnPosition priorPosition;
		Utils::FrameTimer frameTimer;

		bool hasPriorState = false;
		Agent::State priorState;
		float priorAction;
	};
}
