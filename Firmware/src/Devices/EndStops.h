#pragma once
#include "stdint.h"
#include "driver/gpio.h"
#include "driver/pcnt.h"

namespace Devices {
	class EndStops {
	public:
		void init();
		void update();
	private:
		pcnt_unit_t pcntUnit;
	};
}
