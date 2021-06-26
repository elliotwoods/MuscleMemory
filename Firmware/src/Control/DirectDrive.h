#pragma once

#include "FilteredTarget.h"
#include "Utils/FrameTimer.h"
#include "DataTypes.h"

namespace Control {
	class DirectDrive {
	public:
		DirectDrive();
		void init();
		void update();
	};
}