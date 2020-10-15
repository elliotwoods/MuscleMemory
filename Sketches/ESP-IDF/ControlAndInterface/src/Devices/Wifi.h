#pragma once

namespace Devices {
	class Wifi {
	private:
		Wifi() {}
	public:
		static Wifi & X();
		void init();
	};
}

