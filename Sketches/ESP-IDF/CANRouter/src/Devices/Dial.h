#pragma once
#include "stdint.h"
#include "driver/gpio.h"
#include "driver/pcnt.h"

namespace Devices {
	class Dial {
	public:
		void init(gpio_num_t pinA = gpio_num_t::GPIO_NUM_34, gpio_num_t pinB = gpio_num_t::GPIO_NUM_35);
		int16_t getPosition();
	private:
		pcnt_unit_t pcntUnit;
	};
}
