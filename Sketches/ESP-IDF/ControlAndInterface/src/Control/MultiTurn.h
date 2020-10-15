#pragma once
#include "EncoderCalibration.h"
#include "../Devices/AS5047.h"
#include "../DataTypes.h"

namespace Control {
	class MultiTurn {
	public:
		MultiTurn(Devices::AS5047 &, EncoderCalibration &);
		void init();
		void update();
		int32_t getPosition() const;
	private:
		Devices::AS5047 & as5047;
		EncoderCalibration & encoderCalibration;
		uint16_t priorEncoderReading = 0;
		int16_t turns = 0;
		int32_t position = 0;
	};
}