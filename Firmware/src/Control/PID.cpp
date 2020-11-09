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

		// Get error on position (we scale down to get it closer to torque values)
		int32_t rawErrorOnPosition = (agentReads.targetPosition - agentReads.multiTurnPosition);
		auto scaledErrorOnPosition = rawErrorOnPosition >> 4;

		// These values aren't semaphored! but they do change slowly so should be fine
		const auto & kP = registry.registers.at(Registry::RegisterType::PIDProportional).value;
		const auto & kD = registry.registers.at(Registry::RegisterType::PIDDifferential).value;
		const auto & kI = registry.registers.at(Registry::RegisterType::PIDIntegral).value;

		// Calculate ID 
		auto integral = this->priorIntegral + scaledErrorOnPosition * dt / 1000000; // normalise for seconds
		auto derivative =  (scaledErrorOnPosition - this->priorError) * 1000000 / dt; // derivative in seconds, reduce scale

		// clamp integral
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

		auto output = kP * scaledErrorOnPosition
			+ kD * derivative
			+ kI * integral / 1000000; // Normalise for seconds

		// Scale output
		output = output >> 20;

		// Apply anti-stall
		if(registry.registers.at(Registry::RegisterType::AntiStallEnabled).value) {
			auto & antiStallValue = registry.registers.at(Registry::RegisterType::AntiStallValue).value;

			// Check dead zone
			if(abs(rawErrorOnPosition) <= registry.registers.at(Registry::RegisterType::AntiStallDeadZone).value) {
				// Inside dead zone, zero the anti-stall and don't apply it
				antiStallValue = 0;

				// Also turn off torque
				output = 0;
			}
			else {
				const auto speed = abs(agentReads.velocity);

				// Attack if speed too low
				if(speed < registry.registers.at(Registry::RegisterType::AntiStallMinVelocity).value) {
					const auto & attack = registry.registers.at(Registry::RegisterType::AntiStallAttack).value;
					// Increase in direction of errorOnPosition
					if(rawErrorOnPosition > 0) {
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
				output += antiStallValue >> registry.registers.at(Registry::RegisterType::AntiStallScale).value;
			}
		}

		// Clamp output and cast down to int8_t
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
			this->priorError = scaledErrorOnPosition;
			this->priorIntegral = integral;
		}

		// Write to registers (not using safe method for now)
		registry.registers.at(Registry::RegisterType::Torque).value = torque;
		registry.registers.at(Registry::RegisterType::AgentControlFrequency).value = this->frameTimer.getFrequency();
		registry.registers.at(Registry::RegisterType::PIDResultP).value = scaledErrorOnPosition;
		registry.registers.at(Registry::RegisterType::PIDResultI).value = integral;
		registry.registers.at(Registry::RegisterType::PIDResultD).value = derivative;
	}
}