#include "DirectDrive.h"
#include "../Registry.h"

namespace Control {
	//----------
	DirectDrive::DirectDrive()
	{

	}

	//----------
	void
	DirectDrive::init()
	{

	}

	//----------
	void
	DirectDrive::update()
	{
		// We move in lock-step at full torque

		// Registry reads
		const auto multiTurnPosition = getRegisterValue(Registry::RegisterType::MultiTurnPosition);
		const auto softLimitMax = getRegisterValue(Registry::RegisterType::SoftLimitMax);
		const auto softLimitMin = getRegisterValue(Registry::RegisterType::SoftLimitMin);
		const auto maximumTorque = getRegisterValue(Registry::RegisterType::MaximumTorque);
		const auto offsetMinimum = getRegisterValue(Registry::RegisterType::OffsetMinimum);
		const auto offsetMaximum = getRegisterValue(Registry::RegisterType::OffsetMaximum);

		// Get the filtered target position
		auto targetPosition = FilteredTarget::X().getTargetFiltered();

		// Clamp the target position
		if(softLimitMax > softLimitMin) {
			if(targetPosition > softLimitMax) {
				targetPosition = softLimitMax;
			}
			if(targetPosition < softLimitMin) {
				targetPosition = softLimitMin;
			}
		}

		// Calculate the drive offset
		// Notes:
		// PositionWithinStepCycle is scaled to 256
		// One step cycle = 4 * Encoder resolution / 200
		int32_t stepCycle = 4 * (1 << 14) / 200;
		int32_t driveOffset = 256 * (targetPosition - multiTurnPosition) / stepCycle;

		// Set torque to max * polarity
		int32_t torque;
		if(driveOffset == 0) {
			torque = 0;
		}
		else if(driveOffset < 0) {
			torque = -maximumTorque;
		}
		else {
			torque = maximumTorque;
		}

		// Set drive offset to abs value
		driveOffset = abs(driveOffset);
		
		// Clamp driveOffset range + detorque if we're within max value
		if(driveOffset < offsetMinimum) {
			torque = torque * driveOffset / offsetMinimum;
		}
		if(driveOffset > offsetMaximum) {
			driveOffset = offsetMaximum;
		}

		setRegisterValue(Registry::RegisterType::Torque, torque);
		setRegisterValue(Registry::RegisterType::DriveOffset, abs(driveOffset));
	}
}