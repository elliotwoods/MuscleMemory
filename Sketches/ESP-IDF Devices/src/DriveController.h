#pragma once

#include "MotorDriver.h"
#include "EncoderCalibration.h"
#include "DataTypes.h"

class DriveController {
public:
	DriveController(MotorDriver &, AS5047 &, EncoderCalibration &);

	void init();
	void applyTorque(Torque,bool);
private:
	MotorDriver & motorDriver;
	AS5047 & as5047;
	EncoderCalibration & encoderCalibration;
};