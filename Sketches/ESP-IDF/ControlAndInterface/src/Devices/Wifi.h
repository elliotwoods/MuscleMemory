#pragma once

extern "C" {
	#include "cJSON.h"
}
#include <string>

namespace Devices {
	class Wifi {
	private:
		Wifi() {}
	public:
		static Wifi & X();
		void init();
		const std::string & getMacAddress() const;
		cJSON * post(const std::string & path, cJSON * content);
	private:
		std::string baseURI;
		std::string macAddress;
	};
}