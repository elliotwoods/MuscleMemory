#include "Drive.h"
#include "Registry.h"

namespace Control {
	//----------
	Drive::Drive(Devices::MotorDriver & motorDriver
		, Devices::AS5047 & as5047
		, EncoderCalibration & encoderCalibration
		, MultiTurn & multiTurn)
	: motorDriver(motorDriver)
	, as5047(as5047)
	, encoderCalibration(encoderCalibration) 
	, multiTurn(multiTurn)
	{
		
	}


	//----------
	void
	Drive::init()
	{
		this->priorPosition = multiTurn.getMultiTurnPosition();
		this->frameTimer.init();
	}

	//----------
	void
	Drive::update()
	{
		static auto & registry = Registry::X();

		this->frameTimer.update();

		auto encoderReading = this->as5047.getPosition();
		this->multiTurn.update(encoderReading);
		auto multiTurnPosition = this->multiTurn.getMultiTurnPosition();
		auto positionWithinStepCycle = this->encoderCalibration.getPositionWithinStepCycle(encoderReading);

		// Calculate velocity
		auto velocity = (multiTurnPosition - this->priorPosition) * 1000000 / this->frameTimer.getPeriod();
		this->priorPosition = multiTurnPosition;

		// read from registry
		Registry::MotorControlReads motorControlReads;
		{
			registry.motorControlRead(motorControlReads);
		}

		// apply torque
		this->motorDriver.setTorque(motorControlReads.torque, positionWithinStepCycle);

		// write to registry
		registry.motorControlWrite({
			encoderReading
			, this->as5047.getErrors()
			, multiTurnPosition
			, velocity
			, this->frameTimer.getFrequency()
		});
	}
}
