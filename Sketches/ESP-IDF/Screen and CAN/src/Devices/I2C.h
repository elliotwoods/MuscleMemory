#include "driver/i2c.h"

#include <set>
#include "stdint.h"

namespace Devices {
	class I2C
	{
	public:
		static I2C &X();
		void init();
		std::set<uint8_t> scan();

		/// Returns true if EDS_OK
		bool perform(i2c_cmd_handle_t);
	};
}