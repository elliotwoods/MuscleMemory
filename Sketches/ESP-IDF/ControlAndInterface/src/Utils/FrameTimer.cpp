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
		this->frequency = 1000000 / this->period;
		this->priorTime = currentTime;
	}

	//----------
	Period
	FrameTimer::getPeriod() const
	{
		return this->period;
	}

	//----------
	Frequency
	FrameTimer::getFrequency() const
	{
		return this->frequency;
	}
}
