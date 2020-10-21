#include "PID.h"

#include "Registry.h"

namespace Control {
	//----------
	void
	PID::init()
	{
		this->frameTimer.init();
	}

	//----------
	// reference : http://robotsforroboticists.com/pid-control/
	// Note this is a simple PID, more advanced one in the Mechaduino example
	void
	PID::update()
	{
		static auto & registry = Registry::X();
		Registry::AgentReads agentReads;
		registry.agentRead(agentReads);
		
		// Get frame time
		this->frameTimer.update();
		auto dt = this->frameTimer.getPeriod();

		// Get error on position
		auto errorOnPosition = agentReads.targetPosition - agentReads.multiTurnPosition;

		auto integral = this->priorIntegral + errorOnPosition * dt;
		auto derivative = (errorOnPosition - this->priorError) / dt;

		// Calculate integral
		{
			const auto & maxIntegral = registry.registers.at(Registry::RegisterType::PIDIntegralMax).value;
			if(integral > maxIntegral) {
				integral = maxIntegral;
			}
			else if(integral < -maxIntegral) {
				integral = -maxIntegral;
			}
			else {
				integral = integral;
			}
		}

		// These values aren't semaphored! but they do change slowly so should be fine
		const auto & kP = registry.registers.at(Registry::RegisterType::PIDProportional).value;
		const auto & kD = registry.registers.at(Registry::RegisterType::PIDDifferential).value;
		const auto & kI = registry.registers.at(Registry::RegisterType::PIDIntegral).value;

		auto output = kP * errorOnPosition
			+ kD * derivative
			+ kI * integral / 1000000; // Normalise for seconds

		// Scale output
		output = output >> 12;
		int8_t torque;
		if(output > agentReads.maximumTorque) {
			torque = agentReads.maximumTorque;
		}
		else if(output < -agentReads.maximumTorque) {
			torque = -agentReads.maximumTorque;
		}
		else {
			torque = output;
		}

		// Apply soft limits
		if(agentReads.multiTurnPosition > agentReads.softLimitMax) {
			torque = -agentReads.maximumTorque;
		}
		else if(agentReads.multiTurnPosition < agentReads.softLimitMin) {
			torque = agentReads.maximumTorque;
		}

		// Store priors
		{
			this->priorError = errorOnPosition;
			this->priorIntegral = integral;
		}

		// Write to registers (not using safe method for now)
		registry.registers.at(Registry::RegisterType::Torque).value = torque;
		registry.registers.at(Registry::RegisterType::AgentControlFrequency).value = this->frameTimer.getFrequency();
		registry.registers.at(Registry::RegisterType::AgentAddConstant).value = this->priorIntegral;
	}
}