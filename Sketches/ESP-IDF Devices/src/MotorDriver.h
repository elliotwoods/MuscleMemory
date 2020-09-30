#pragma once

#include "driver/gpio.h"
#include "driver/dac.h"

#include <math.h>
#include <stdlib.h>

#define PI (acos(0) * 2)

class MotorDriver {
public:
	struct Configuration {
		struct CoilPins {
			gpio_num_t coil_A_positive;
			gpio_num_t coil_A_negative;
			gpio_num_t coil_B_positive;
			gpio_num_t coil_B_negative;
		};

		union {
			CoilPins coilPins;
			gpio_num_t pinArray[4];
		};

		struct VREFDACs {
			dac_channel_t A;
			dac_channel_t B;
		};

		union {
			VREFDACs vrefDACs;
			dac_channel_t dacArray[2];
		};
	};

	MotorDriver(const Configuration & configuration);

	void setTorque(int8_t torque, uint8_t cyclePosition);
private:
	void calculateCosTable();
	Configuration configuration;

	// The cos table stores values for x=0...pi as cosTable[x / PI]
	// For pi...pi*2, take -cosTable[x / PI * 256]
	uint8_t cosTable[256];

	int8_t cosTableFull[256];
};