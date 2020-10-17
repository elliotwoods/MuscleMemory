#pragma once

#include "Devices/INA219.h"

namespace Interface {
	class SystemInfo {
	public:
		SystemInfo(Devices::INA219 &);
		void init();
		void update();
	private:
		Devices::INA219 & ina219;
	};
}