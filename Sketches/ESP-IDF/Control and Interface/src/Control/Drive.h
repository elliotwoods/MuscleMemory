#pragma once

#include "../Devices/MotorDriver.h"
#include "../Devices/AS5047.h"
#include "EncoderCalibration.h"
#include "DataTypes.h"

namespace Control {
	class Drive {
	public:
		Drive(Devices::MotorDriver &, Devices::AS5047 &, EncoderCalibration &);

		void init();
		void applyTorque(Torque, bool debug);
	private:
		Devices::MotorDriver & motorDriver;
		Devices::AS5047 & as5047;
		EncoderCalibration & encoderCalibration;
	};
}
