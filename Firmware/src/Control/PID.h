#pragma once

#include "DataTypes.h"
#include "Utils/FrameTimer.h"

namespace Control {
	class PID {
	public:
		void init();
		void update();
	private:
		Utils::FrameTimer frameTimer;
		int64_t priorIntegral = 0;
		int32_t priorError = 0;
	};
}