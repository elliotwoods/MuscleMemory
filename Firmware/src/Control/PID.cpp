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

		// Apply anti-stall
		if(registry.registers.at(Registry::RegisterType::AntiStallEnabled).value) {
			auto & antiStallValue = registry.registers.at(Registry::RegisterType::AntiStallValue).value;

			// Check dead zone
			if(abs(errorOnPosition) <= registry.registers.at(Registry::RegisterType::AntiStallDeadZone).value) {
				// Inside dead zone, zero the anti-stall and don't apply it
				antiStallValue = 0;
			}
			else {
				const auto speed = abs(agentReads.velocity);

				// Attack if speed too low
				if(speed < registry.registers.at(Registry::RegisterType::AntiStallMinVelocity).value) {
					const auto & attack = registry.registers.at(Registry::RegisterType::AntiStallAttack).value;
					// Increase in direction of errorOnPosition
					if(errorOnPosition > 0) {
						antiStallValue += attack;
					}
					else {
						antiStallValue -= attack;
					}
				}
				else {
					const auto & decay = registry.registers.at(Registry::RegisterType::AntiStallDecay).value;
					// Decay if speed is above threshold
					if(antiStallValue > 0) {
						antiStallValue -= decay;

						// Clamp to 0
						if(antiStallValue < 0) {
							antiStallValue = 0;
						}
					}
					else {
						antiStallValue += decay;

						// Clamp to 0
						if(antiStallValue > 0) {
							antiStallValue = 0;
						}
					}
				}

				// Apply anti-stall
				torque += antiStallValue >> 4;
			}
		}

		// Clamp output
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