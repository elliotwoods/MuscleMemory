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
	FilteredTarget::getTargetFiltered() const
	{
		const auto now = esp_timer_get_time();
		const auto timeDelta = (int32_t) (now - this->priorTargetTimestamp);
		if(timeDelta > this->maxTimeDelta) {
			// If we haven't receiving any updates within X seconds, don't perform filtering
			return this->priorTarget;
		}
		else {
			return this->filterVelocity * timeDelta / 1000 + this->priorTarget;
		}
	}

	//-----------
	FilteredTarget::FilteredTarget()
	{

	}
}