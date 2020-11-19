#pragma once

#include "EncoderCalibration.h"
#include "../Devices/MotorDriver.h"
#include "../Devices/INA219.h"
#include "../Devices/AS5047.h"
#include "../Devices/FileSystem.h"

#include "Interface/CANResponder.h"
#include "Interface/WebSockets.h"

namespace Control {
	class Provisioning {
	public:
		Provisioning(Devices::MotorDriver &
			, Devices::INA219 &
			, Devices::AS5047 &
			, Devices::FileSystem &
			, EncoderCalibration &
			, Interface::CANResponder &
			, Interface::WebSockets &
			);
		void perform();
	
		struct Settings {
			uint8_t maxCurrent = 128;
			uint8_t current	= 16;
			int speed = 0;
			bool direction = true;
		};

		struct Status {
			float voltage;
			float current;
			uint16_t encoderReading;
			float encoderReadingNormalised;
		};

		Devices::MotorDriver & motorDriver;
		Devices::INA219 & ina219;
		Devices::AS5047 & as5047;
		Devices::FileSystem & fileSystem;
		
		EncoderCalibration & encoderCalibration;

		Interface::CANResponder & canResponder;
		Interface::WebSockets & webSockets;

		Settings settings;
		Status status;

		int32_t stepIndex = 0;
		bool shouldExit = false;
	};
}