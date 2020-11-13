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
	void IRAM_ATTR
	PID::update()
	{
		// Registry reads
		const auto multiTurnPosition = getRegisterValue(Registry::RegisterType::MultiTurnPosition);
		const auto kP = (int64_t) getRegisterValue(Registry::RegisterType::PIDProportional);
		const auto kD = (int64_t) getRegisterValue(Registry::RegisterType::PIDDifferential);
		const auto kI = (int64_t) getRegisterValue(Registry::RegisterType::PIDIntegral);
		const auto integralMax = (int64_t) getRegisterValue(Registry::RegisterType::PIDIntegralMax);
		const auto antiStallEnabled = getRegisterValue(Registry::RegisterType::AntiStallEnabled) == 1;
		const auto softLimitMax = getRegisterValue(Registry::RegisterType::SoftLimitMax);
		const auto softLimitMin = getRegisterValue(Registry::RegisterType::SoftLimitMin);
		const auto maximumTorque = getRegisterValue(Registry::RegisterType::MaximumTorque);

		// Get the filtered target position
		const auto targetPosition = this->filteredTarget.getTargetFiltered();

		// Get frame time
		this->frameTimer.update();
		auto dt = (int64_t) this->frameTimer.getPeriod();

		// Get error on position (we scale down to get it closer to torque values)
		const int64_t pidPositionScale = 1 << 4;
		auto rawErrorOnPosition = (int64_t) targetPosition - (int64_t) multiTurnPosition;
		auto scaledErrorOnPosition = rawErrorOnPosition / pidPositionScale;

		// Calculate ID 
		auto integral = this->priorIntegral + rawErrorOnPosition * dt / 1000000LL; // normalise for seconds
		auto derivative =  (scaledErrorOnPosition - this->priorError) * 1000000LL / dt; // derivative in seconds, reduce scale

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
			+ kI * integral / 1000000LL; // Normalise for seconds

		// Scale output
		auto torque = (int32_t) (output / (1 << 20));

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
				torque = 0;
			}
			else {
				const auto speed = (int32_t) abs(derivative);

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
				torque += antiStallValue / (1 << antiStallScale);
			}

			// Store anti-stall value
			setRegisterValue(Registry::RegisterType::AntiStallValue, antiStallValue);
		}

		// Clamp output and cast down to int8_t
		if(torque > maximumTorque) {
			torque = maximumTorque;
		}
		else if(torque < -maximumTorque) {
			torque = -maximumTorque;
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

		// Set the offset
		int32_t offset;
		{
			const auto offsetFactor = getRegisterValue(Registry::RegisterType::OffsetFactor);
			const auto offsetMaximum = getRegisterValue(Registry::RegisterType::OffsetMaximum);
			offset = (int32_t) abs(rawErrorOnPosition / (int64_t) offsetFactor);
			if(offset > offsetMaximum) {
				offset = offsetMaximum;
			}
		}

		// Write to registers (not using safe method for now)
		setRegisterValue(Registry::RegisterType::Torque, torque);
		setRegisterValue(Registry::RegisterType::AgentControlFrequency, this->frameTimer.getFrequency());
		setRegisterValue(Registry::RegisterType::PIDResultP, scaledErrorOnPosition);
		setRegisterValue(Registry::RegisterType::PIDResultI, integral);
		setRegisterValue(Registry::RegisterType::PIDResultD, derivative);
		setRegisterValue(Registry::RegisterType::DriveOffset, offset);
	}
}