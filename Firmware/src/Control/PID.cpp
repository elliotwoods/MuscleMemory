#include "PID.h"

#include "Registry.h"

namespace Control {
	//----------
	PID::PID(FilteredTarget & filteredTarget)
	: filteredTarget(filteredTarget)
	{

	}
	
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
		// Registry reads
		const auto & multiTurnPosition = getRegisterValue(Registry::RegisterType::MultiTurnPosition);
		const auto & kP = getRegisterValue(Registry::RegisterType::PIDProportional);
		const auto & kD = getRegisterValue(Registry::RegisterType::PIDDifferential);
		const auto & kI = getRegisterValue(Registry::RegisterType::PIDIntegral);
		const auto & integralMax = getRegisterValue(Registry::RegisterType::PIDIntegralMax);
		const auto & antiStallEnabled = getRegisterValue(Registry::RegisterType::AntiStallEnabled);
		const auto & softLimitMax = getRegisterValue(Registry::RegisterType::SoftLimitMax);
		const auto & softLimitMin = getRegisterValue(Registry::RegisterType::SoftLimitMin);
		const auto & maximumTorque = getRegisterValue(Registry::RegisterType::MaximumTorque);

		// Get the filtered target position
		const auto targetPosition = this->filteredTarget.getTargetFiltered();

		// Get frame time
		this->frameTimer.update();
		auto dt = this->frameTimer.getPeriod();

		// Get error on position (we scale down to get it closer to torque values)
		int32_t rawErrorOnPosition = (targetPosition - multiTurnPosition);
		auto scaledErrorOnPosition = rawErrorOnPosition >> 4;

		// Calculate ID 
		auto integral = this->priorIntegral + scaledErrorOnPosition * dt / 1000000; // normalise for seconds
		auto derivative =  (scaledErrorOnPosition - this->priorError) * 1000000 / dt; // derivative in seconds, reduce scale

		// clamp integral
		{
			if(integral > integralMax) {
				integral = integralMax;
			}
			else if(integral < -integralMax) {
				integral = -integralMax;
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
		if(antiStallEnabled) {
			const auto antiStallDeadZone = getRegisterValue(Registry::RegisterType::AntiStallDeadZone);
			const auto antiStallMinVelocity = getRegisterValue(Registry::RegisterType::AntiStallMinVelocity);
			const auto antiStallAttack = getRegisterValue(Registry::RegisterType::AntiStallAttack);
			const auto antiStallDecay = getRegisterValue(Registry::RegisterType::AntiStallDecay);
			const auto antiStallScale = getRegisterValue(Registry::RegisterType::AntiStallScale);

			auto antiStallValue = getRegisterValue(Registry::RegisterType::AntiStallValue);

			// Check dead zone
			if(abs(rawErrorOnPosition) <= antiStallDeadZone) {
				// Inside dead zone, zero the anti-stall and don't apply it
				antiStallValue = 0;

				// Also turn off torque
				output = 0;
			}
			else {
				const auto speed = abs(derivative);

				// Attack if speed too low
				if(speed < antiStallMinVelocity) {
					// Increase in direction of errorOnPosition
					if(rawErrorOnPosition > 0) {
						antiStallValue += antiStallAttack;
					}
					else {
						antiStallValue -= antiStallAttack;
					}
				}
				else {
					// Decay if speed is above threshold
					if(antiStallValue > 0) {
						antiStallValue -= antiStallDecay;

						// Clamp to 0
						if(antiStallValue < 0) {
							antiStallValue = 0;
						}
					}
					else {
						antiStallValue += antiStallDecay;

						// Clamp to 0
						if(antiStallValue > 0) {
							antiStallValue = 0;
						}
					}
				}

				// Kill anti-stall if it's in the wrong direction
				if((rawErrorOnPosition > 0) != (antiStallValue > 0)) {
					antiStallValue = 0;
				}

				// Apply anti-stall
				output += antiStallValue >> antiStallScale;
			}

			// Store anti-stall value
			setRegisterValue(Registry::RegisterType::AntiStallValue, antiStallValue);
		}

		// Clamp output and cast down to int8_t
		int8_t torque;
		if(output > maximumTorque) {
			torque = maximumTorque;
		}
		else if(output < -maximumTorque) {
			torque = -maximumTorque;
		}
		else {
			torque = output;
		}

		// Apply soft limits
		if(multiTurnPosition > softLimitMax) {
			torque = -maximumTorque;
		}
		else if(multiTurnPosition < softLimitMin) {
			torque = maximumTorque;
		}

		// Store priors
		{
			this->priorError = scaledErrorOnPosition;
			this->priorIntegral = integral;
		}

		// Write to registers (not using safe method for now)
		setRegisterValue(Registry::RegisterType::Torque, torque);
		setRegisterValue(Registry::RegisterType::AgentControlFrequency, this->frameTimer.getFrequency());
		setRegisterValue(Registry::RegisterType::PIDResultP, scaledErrorOnPosition);
		setRegisterValue(Registry::RegisterType::PIDResultI, integral);
		setRegisterValue(Registry::RegisterType::PIDResultD, derivative);
	}
}