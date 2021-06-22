#include "FilteredTarget.h"
#include "Registry.h"

extern "C" {
	#include "esp_timer.h"
	#include "esp_attr.h"
}

namespace Control {
	//----------
	FilteredTarget &
	FilteredTarget::X()
	{
		static auto x = FilteredTarget();
		return x;
	}

	//----------
	void
	FilteredTarget::update()
	{
		setRegisterValue(Registry::RegisterType::TargetPositionFiltered, this->getTargetFiltered());
	}

	//----------
	void IRAM_ATTR
	FilteredTarget::notifyTargetChange(int32_t targetPosition)
	{
		this->priorTarget = targetPosition;
		this->priorTargetTimestamp = esp_timer_get_time();
	}

	//----------
	MultiTurnPosition IRAM_ATTR
	FilteredTarget::getTargetFiltered()
	{
		const auto now = esp_timer_get_time();

		const auto currentPosition = getRegisterValue(Registry::RegisterType::MultiTurnPosition);

		int32_t target;
		// select target based on how recently a new target has been set
		{
			const auto timeSinceLastSetTargetPosition = (int32_t) (now - this->priorTargetTimestamp);
			if(timeSinceLastSetTargetPosition > this->maxTimeDelta) {
				// If we haven't receiving any updates within X seconds, don't perform filtering
				target = this->priorTarget;
			}
			else {
				// TargetVelocity is in Position/second. Time is in microseconds
				target = getRegisterValue(Registry::RegisterType::TargetVelocity) * timeSinceLastSetTargetPosition / 1000000 + this->priorTarget;
			}
		}
		
		// clamp target position by max velocity
		const auto maxVelocity = getRegisterValue(Registry::RegisterType::MaxVelocity);
		if(maxVelocity > 0) {
			const auto timeSinceLastUpdate = now - this->priorUpdateTimestamp;
			const auto deltaLimit = (int64_t) maxVelocity * timeSinceLastUpdate / 1000000LL;
			if(target - currentPosition > deltaLimit) {
				target = currentPosition + (int32_t) deltaLimit;
			}
			else if(target - currentPosition < - (int32_t) deltaLimit) {
				target = currentPosition - deltaLimit;
			}
		}

		this->priorUpdateTimestamp = now;

		return target;
	}

	//-----------
	FilteredTarget::FilteredTarget()
	{

	}
}