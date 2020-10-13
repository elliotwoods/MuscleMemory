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
DriveController::applyTorque(Torque torque, bool track)
{
	auto encoderReading = this->as5047.getPosition();
	auto positionWithinStepCycle = this->encoderCalibration.getPositionWithinStepCycle(encoderReading);
	this->motorDriver.setTorque(torque, positionWithinStepCycle);
	if(track){
		printf("EncoderReading : \t %d", encoderReading);
		printf("\t PositionWithinStepCycle: \t %d \n", positionWithinStepCycle);
	}
}