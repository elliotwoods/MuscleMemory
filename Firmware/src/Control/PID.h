#pragma once

#include "FilteredTarget.h"
#include "Utils/FrameTimer.h"
#include "DataTypes.h"

namespace Control {
	class PID {
	public:
		PID();
		void init();
		void update();
	private:
		Utils::FrameTimer frameTimer;
		int64_t priorIntegral = 0;
		int64_t priorError = 0;
	};
}