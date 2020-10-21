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
			const auto & deviceID = Registry::X().registers.at(Registry::RegisterType::DeviceID).value;
			
			char message[100];
			
			u8g2.setFont(u8g2_font_nerhoe_tr ); 
			sprintf(message, "Device ID [0x%X]", deviceID);
			u8g2.drawStr(3, 12, message);	

			u8g2.setFont(u8g2_font_logisoso34_tr );
			sprintf(message, "%d", deviceID);
			u8g2.drawStr(40, 52, message);

			if(this->editing) {
				u8g2.drawFrame(0, 0, 128, 64);
			}
		}

		
		//----------
		bool
		ShowID::buttonPressed()
		{
			if(!this->editing) {
				this->editing = true;
			}
			else {
				Registry::X().saveDefault(Registry::RegisterType::DeviceID);
				this->editing = false;
				this->shouldExit = true;
			}

			// we manually clear the panel
			return false;
		}

		
		//----------
		void
		ShowID::dial(int8_t movements)
		{
			if(this->editing) {
				auto & reg = Registry::X().registers.at(Registry::RegisterType::DeviceID);
				auto & deviceID = reg.value;
				deviceID += movements;
				if(deviceID < reg.range.min) {
					deviceID = reg.range.max;
				}
				else if(deviceID > reg.range.max) {
					deviceID = reg.range.min;
				}
			}
		}
	}
}