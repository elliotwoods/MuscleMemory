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
		this->frameTimer.update();

		// read from registry
		const auto encoderPositionFilterSize = getRegisterValue(Registry::RegisterType::EncoderPositionFilterSize);
		const auto torque = getRegisterValue(Registry::RegisterType::Torque);

		// Get the encoder reading (using averaging if enabled)
		auto encoderReading = encoderPositionFilterSize > 1
			? this->as5047.getPositionFiltered(encoderPositionFilterSize)
			: this->as5047.getPosition();
		
		this->multiTurn.driveLoopUpdate(encoderReading);
		auto multiTurnPosition = this->multiTurn.getMultiTurnPosition();
		auto positionWithinStepCycle = this->encoderCalibration.getPositionWithinStepCycle(encoderReading);

		// Calculate velocity
		auto velocity = (multiTurnPosition - this->priorPosition) * 1000000 / this->frameTimer.getPeriod();
		this->priorPosition = multiTurnPosition;

		// apply torque
		this->motorDriver.setTorque(torque, positionWithinStepCycle);

		// write to registry
		{
			setRegisterValue(Registry::RegisterType::EncoderReading, encoderReading);
			setRegisterValue(Registry::RegisterType::EncoderErrors, this->as5047.getErrors());
			setRegisterValue(Registry::RegisterType::MultiTurnPosition, multiTurnPosition);
			setRegisterValue(Registry::RegisterType::Velocity, velocity);
			setRegisterValue(Registry::RegisterType::MotorControlFrequency, this->frameTimer.getFrequency());
		}
	}
}
