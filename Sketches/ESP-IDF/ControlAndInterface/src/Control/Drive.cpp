#include "Drive.h"
#include "Registry.h"

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
		static auto & registry = Registry::X();

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
			state.targetMinusPosition = controlLoopReads.targetPosition - multiTurnPosition;

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
			int32_t reward = abs(state.targetMinusPosition);
			this->agent.recordTrajectory({
				this->priorState
				, this->priorAction
				, (float) reward
				, state
			});
		}
		
		auto action = this->agent.selectAction(state);

		// Perform action as torque
		int8_t torque;
		{
			auto actionIn8BitRange = action * 128.0f;

			// clip to max torque values
			actionIn8BitRange = fmax(fmin(actionIn8BitRange, (float) controlLoopReads.maximumTorque - 1.0f), -(float) controlLoopReads.maximumTorque);

			torque = (int8_t) actionIn8BitRange;
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
			, torque
			, state.velocity
		});
	}
}
