#pragma once

#include "stdint.h"

namespace Utils {
	class FrameTimer {
	public:
		void init();
		void update();
		int32_t getPeriod() const; // us
		int32_t getFrequency() const; // Hz
	private:
		int64_t priorTime;
		int32_t period;
		int32_t frequency;
	};
}