#pragma once

#include "driver/gpio.h"
#include "driver/dac.h"

#include <math.h>
#include <stdlib.h>

#include "DataTypes.h"

#ifndef PI
#define PI (acos(0) * 2)
#endif

namespace Devices {
	class MotorDriver {
	public:	
		struct Configuration {
			Configuration();
			
			struct CoilPins {
				gpio_num_t coil_A_positive;
				gpio_num_t coil_B_positive;
				gpio_num_t coil_A_negative;
				gpio_num_t coil_B_negative;
			};

			union {
				CoilPins coilPins;
				gpio_num_t coilPinArray[4];
			};

			struct VREFDACs {
				dac_channel_t A;
				dac_channel_t B;
			};

			union {
				VREFDACs vrefDacs;
				dac_channel_t vrefDacArray[2];
			};
		};

		void init(const Configuration & = Configuration());

		/// Apply a force with magnitude and direction defined by torque
		void setTorque(Torque, PositionWithinStepCycle);

		/// Perform a traditional step (This is not used during runtime).
		/// index = 0...3
		void step(uint8_t index, uint8_t current);
	private:
		void calculateCosTable();
		Configuration configuration;

		// The cos table stores values for x=0...pi as cosTable[x / PI]
		// For pi...pi*2, take -cosTable[x / PI * 256]
		uint8_t cosTable[256];

		int8_t cosTableFull[256];
	};
}