#pragma once

#include "FilteredTarget.h"
#include "Utils/FrameTimer.h"
#include "DataTypes.h"

namespace Control {
	class PID {
	public:
		PID(FilteredTarget &);
		void init();
		void update();
	private:
		FilteredTarget & filteredTarget;
		Utils::FrameTimer frameTimer;
		int64_t priorIntegral = 0;
		int32_t priorError = 0;
	};
}