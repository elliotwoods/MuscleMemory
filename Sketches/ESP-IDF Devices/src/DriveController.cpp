#include "DriveController.h"

//----------
DriveController::DriveController(MotorDriver & motorDriver
	, AS5047 & as5047
	, EncoderCalibration & encoderCalibration)
: motorDriver(motorDriver)
, as5047(as5047)
, encoderCalibration(encoderCalibration) 
{
	
}


//----------
void
DriveController::init()
{
	
}

//----------
void
DriveController::applyTorque(Torque torque)
{
	auto encoderReading = this->as5047.getPosition();
	auto positionWithinStepCycle = this->encoderCalibration.getPositionWithinStepCycle(encoderReading);
	this->motorDriver.setTorque(torque, positionWithinStepCycle);

	//printf("EncoderReading : %d\n", as5047.getPosition());
	//printf("PositionWithinStepCycle: %d\n", positionWithinStepCycle);
}