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
			
			u8g2.setFont(u8g2_font_nerhoe_tr ); 
			char message[200];
			sprintf(message, "DEVICE");
			u8g2.drawStr(20, 35, message);	

			u8g2.setFont(u8g2_font_fub35_tn);
			sprintf(message, "%d", deviceID.value);
			int IDxPosition = 75;
			if(deviceID.value>99){
				IDxPosition+=20;
			}else if(deviceID.value>9){
				IDxPosition+=10;
			}
			u8g2.drawStr(IDxPosition, 50, message);
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