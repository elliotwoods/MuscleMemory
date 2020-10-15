#pragma once

#include "MultiTurn.h"
#include "Agent.h"
#include "../Devices/MotorDriver.h"
#include "../Devices/AS5047.h"
#include "EncoderCalibration.h"
#include "DataTypes.h"

namespace Control {
	class Drive {
	public:
		Drive(Devices::MotorDriver &
			, Devices::AS5047 &
			, EncoderCalibration &
			, MultiTurn &
			, Agent &);

		void init();
		void update();
	private:
		Devices::MotorDriver & motorDriver;
		Devices::AS5047 & as5047;
		EncoderCalibration & encoderCalibration;
		MultiTurn & multiTurn;
		Agent & agent;
	};
}
