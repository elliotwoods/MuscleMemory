#include "Drive.h"
#include "Registry.h"

auto & registry = Registry::X();

namespace Control {
	//----------
	Drive::Drive(Devices::MotorDriver & motorDriver
		, Devices::AS5047 & as5047
		, EncoderCalibration & encoderCalibration)
	: motorDriver(motorDriver)
	, as5047(as5047)
	, encoderCalibration(encoderCalibration) 
	{
		
	}


	//----------
	void
	Drive::init()
	{
		
	}

	//----------
	void
	Drive::applyTorque(Torque torque, bool debug)
	{
		auto encoderReading = this->as5047.getPosition();
		auto positionWithinStepCycle = this->encoderCalibration.getPositionWithinStepCycle(encoderReading);
		this->motorDriver.setTorque(torque, positionWithinStepCycle);
		if(debug){
			printf("EncoderReading : \t %d", encoderReading);
			printf("\t PositionWithinStepCycle: \t %d \n", positionWithinStepCycle);
		}

		registry.controlLoopWrite({
			encoderReading
			, this->as5047.getErrors()
			, 0
		});
	}
}
