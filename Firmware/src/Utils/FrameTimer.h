#pragma once

#include "stdint.h"
#include "DataTypes.h"

namespace Utils {
	class FrameTimer {
	public:
		void init();
		void update();
		Period getPeriod() const; // us
		Frequency getFrequency() const; // Hz
	private:
		int64_t priorTime;
		Period period;
		Frequency frequency;
	};
}