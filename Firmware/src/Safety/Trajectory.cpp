#include "Trajectory.h"
#include "Registry.h"
#include "../Control/FilteredTarget.h"

namespace Safety {
	//----------
	void
	Trajectory::init()
	{

	}

	//----------
	void
	Trajectory::update()
	{
		static const auto & controlMode = getRegisterValue(Registry::RegisterType::ControlMode);

		if(controlMode > 0) {
			// Check how far we are off our target
			const auto targetPosition = Control::FilteredTarget::X().getTargetFiltered();
			static const auto & multiTurnPosition = getRegisterValue(Registry::RegisterType::MultiTurnPosition);
			static const auto & maxPositionDeviation = getRegisterValue(Registry::RegisterType::MaxPositionDeviation);
			const auto positionDelta = multiTurnPosition - targetPosition;

			if(abs(positionDelta) > maxPositionDeviation) {
				// we've deviated too far - disengage control
				setRegisterValue(Registry::RegisterType::ControlMode, 0);

				// TODO : make a more consistent safety issue, e.g. announce on CAN bus
			}
		}		
	}
}