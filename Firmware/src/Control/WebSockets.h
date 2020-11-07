#pragma once

#include "Registry.h"
#include <stdint.h>
#include <cstring>
#include <msgpack.h>

namespace Control {
	class WebSockets {
	public:
		void init();
		void update();
		void processIncomingRequests(uint8_t *, size_t);
		void processIncomingAction(const char * name, const msgpack_object &);
		void processIncomingRegisterValue(const Registry::RegisterType &, const msgpack_object &);
	private:
		bool needsSendRegisterInfo = true;
		void sendRegisterInfo();
		void sendRegisters();
	};
}