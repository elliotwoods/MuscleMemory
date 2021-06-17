#pragma once

#include "Devices/CurrentSensor.h"

namespace Interface {
	class SystemInfo {
	public:
		SystemInfo(Devices::CurrentSensor &);
		void init();
		void update();
	private:
		Devices::CurrentSensor & currentSensor;
	};
}