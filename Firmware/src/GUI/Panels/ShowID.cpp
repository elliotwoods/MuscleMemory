#include "ShowID.h"
#include "Registry.h"

namespace GUI {
	namespace Panels {
		//----------
		ShowID::ShowID()
		{
			this->startTime = esp_timer_get_time();
		}

		//----------
		void
		ShowID::update()
		{
			auto timeShown = esp_timer_get_time() - this->startTime;
			if(!editing && timeShown > 1000000) {
				this->shouldExit = true;
			}
		}

		//----------
		void
		ShowID::draw(U8G2& u8g2)
		{
			const auto & value = Registry::X().registers.at(Registry::RegisterType::DeviceID).value;
			
			char message[100];
			
			u8g2.setFont(u8g2_font_nerhoe_tr ); 
			sprintf(message, "Device ID [0x%X]", value);
			u8g2.drawStr(3, 12, message);	

			u8g2.setFont(u8g2_font_fub35_tn);
			sprintf(message, "%d", value);
			int IDxPosition = 93;
			auto pxWidth = int(log(value) / log(10)) * 30;

			u8g2.drawStr(IDxPosition - pxWidth, 53, message);

			if(this->editing) {
				u8g2.drawFrame(0, 0, 128, 64);
			}
		}

		
		//----------
		void
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