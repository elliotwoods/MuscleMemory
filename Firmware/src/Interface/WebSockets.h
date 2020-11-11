#pragma once

#include "Control/EncoderCalibration.h"

#include "Registry.h"
#include <stdint.h>
#include <cstring>
#include <msgpack.h>

namespace Interface {
	class WebSockets {
	public:
		WebSockets(const Control::EncoderCalibration & encoderCalibration);
		void init();
		void update();
		void processIncomingRequests(uint8_t *, size_t);
		void processIncomingAction(const char * name, const msgpack_object &);
		void processIncomingRegisterValue(const Registry::RegisterType &, const msgpack_object &);
	private:
		void sendRegisterInfo();
		void sendRegisters();
		void sendEncoderCalibration();

		bool needsSendRegisterInfo = true;
		bool needsSendEncoderCalibration = false;

		const Control::EncoderCalibration & encoderCalibration;
	};
}