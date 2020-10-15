#include "Drive.h"
#include "Registry.h"
auto & registry = Registry::X();

namespace Control {
	//----------
	Drive::Drive(Devices::MotorDriver & motorDriver
		, Devices::AS5047 & as5047
		, EncoderCalibration & encoderCalibration
		, MultiTurn & multiTurn
		, Agent & agent)
	: motorDriver(motorDriver)
	, as5047(as5047)
	, encoderCalibration(encoderCalibration) 
	, multiTurn(multiTurn)
	, agent(agent)
	{
		
	}


	//----------
	void
	Drive::init()
	{

	}

	//----------
	void
	Drive::update()
	{
		auto encoderReading = this->as5047.getPosition();
		auto positionWithinStepCycle = this->encoderCalibration.getPositionWithinStepCycle(encoderReading);

		int8_t torque;

		this->motorDriver.setTorque(torque, positionWithinStepCycle);

		registry.controlLoopWrite({
			encoderReading
			, this->as5047.getErrors()
			, 0
		});
	}
}
