#include "ShowID.h"
#include "Registry.h"

namespace GUI {
	namespace Panels {
		//----------
		ShowID::ShowID()
		{

		}

		//----------
		void
		ShowID::update()
		{

		}

		//----------
		void
		ShowID::draw(U8G2& u8g2)
		{
			auto deviceID = Registry::X().registers.at(Registry::RegisterType::DeviceID);

		}

		
		//----------
		bool
		ShowID::buttonPressed()
		{
			return false;
		}

		
		//----------
		void
		ShowID::dial(int8_t)
		{
			
		}
	}
}