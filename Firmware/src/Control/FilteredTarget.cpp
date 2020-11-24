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
		if(this->targetChange) {
			auto newTarget = getRegisterValue(Registry::RegisterType::TargetPosition);
			auto now = esp_timer_get_time();
			this->filterVelocity = (newTarget - this->priorTarget) * 1000 / (int32_t) (now - this->priorTargetTimestamp);
			this->priorTarget = newTarget;
			this->priorTargetTimestamp = now;
			this->targetChange = false;
		}
		setRegisterValue(Registry::RegisterType::TargetPositionFiltered, this->getTargetFiltered());
	}

	//----------
	void IRAM_ATTR
	FilteredTarget::notifyTargetChange()
	{
		this->targetChange = true;
	}

	//----------
	MultiTurnPosition IRAM_ATTR
	FilteredTarget::getTargetFiltered()
	{
		const auto now = esp_timer_get_time();

		const auto maxVelocity = getRegisterValue(Registry::RegisterType::MaxVelocity);
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
				target = this->filterVelocity * timeSinceLastSetTargetPosition / 1000 + this->priorTarget;
			}
		}
		
		// clamp velocity
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