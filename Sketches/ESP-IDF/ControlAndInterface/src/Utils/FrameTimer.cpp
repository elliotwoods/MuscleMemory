#include "FrameTimer.h"

#include "esp_timer.h"

namespace Utils
{
	//----------
	void
	FrameTimer::init()
	{
		this->priorTime = esp_timer_get_time();
	}

	//----------
	void
	FrameTimer::update()
	{
		auto currentTime = esp_timer_get_time();
		this->period = currentTime - this->priorTime;
		this->frequency = 1000000L / this->period;
		this->priorTime = currentTime;
	}

	//----------
	int32_t
	FrameTimer::getPeriod() const
	{
		return this->period;
	}

	//----------
	int32_t
	FrameTimer::getFrequency() const
	{
		return this->frequency;
	}
}
