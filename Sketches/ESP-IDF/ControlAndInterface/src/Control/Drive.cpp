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
		this->priorPosition = multiTurn.getMultiTurnPosition();
		this->priorTime = esp_timer_get_time();
	}

	//----------
	void
	Drive::update()
	{
		auto encoderReading = this->as5047.getPosition();
		this->multiTurn.update(encoderReading);
		auto multiTurnPosition = this->multiTurn.getMultiTurnPosition();
		auto positionWithinStepCycle = this->encoderCalibration.getPositionWithinStepCycle(encoderReading);

		// read from registry
		Registry::ControlLoopReads controlLoopReads;
		{
			registry.controlLoopRead(controlLoopReads);
		}

		// Prepare the state
		Agent::State state;
		{
			state.position = multiTurnPosition;
			state.target = controlLoopReads.targetPosition;

			// Calculate frequency and velocity
			{
				auto currentTime = esp_timer_get_time();
				auto period = currentTime - priorTime;
				state.frequency = 1e6 / period;
				this->priorTime = currentTime;

				state.velocity = (multiTurnPosition - this->priorPosition) * 1e6 / period;
				this->priorPosition = multiTurnPosition;
			}
		}

		// Record trajectory
		if(this->hasPriorState) {
			int32_t reward = abs(multiTurnPosition - controlLoopReads.targetPosition);
			this->agent.recordTrajectory(this->priorState, this->priorAction, reward, state);
		}
		
		auto action = this->agent.selectAction(state);

		// Perform action as torque
		{
			auto actionIn8BitRange = (action - 0.5f)* 255.0f - 128.0f;
			auto actionClipped = fmax(fmin(actionIn8BitRange, 127), -128);
			int8_t torque = (int8_t) actionClipped;
			this->motorDriver.setTorque(torque, positionWithinStepCycle);
		}

		// Remember trajectory variables
		{
			std::swap(this->priorState, state);
			this->priorAction = action;
			this->hasPriorState = true;
		}

		registry.controlLoopWrite({
			encoderReading
			, this->as5047.getErrors()
			, multiTurnPosition
		});
	}
}
