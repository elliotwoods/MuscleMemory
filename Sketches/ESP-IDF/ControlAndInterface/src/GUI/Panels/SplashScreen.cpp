#include "SplashScreen.h"
#include "Logo.h"

namespace GUI {
	namespace Panels {
		//----------
		SplashScreen::SplashScreen()
		{

		}

		//----------
		void
		SplashScreen::update()
		{
		}

		//----------
		void
		SplashScreen::draw(U8G2 & u8g2)
		{
			u8g2.drawXBM((128-KimchipsLogo100_width)/2
						//, (64-KimchipsLogo100_height)/2
						, 0
						, KimchipsLogo100_width
						, KimchipsLogo100_height
						, KimchipsLogo100_bits
						); 
			u8g2.setFont(u8g2_font_nerhoe_tr);
			char message[200];
			sprintf(message, "%s", this->message.c_str());
			u8g2.drawStr(10, 50, message);
		}
		
		//----------
		bool
		SplashScreen::buttonPressed()
		{
			return false;
		}

		//----------
		void
		SplashScreen::dial(int8_t)
		{
			
		}

		//----------
		void
		SplashScreen::setMessage(const std::string& message)
		{
			if(message[0]==' '){
				this->message += message;
			}else{
				this->message = message;
			}
		}
	}
}

