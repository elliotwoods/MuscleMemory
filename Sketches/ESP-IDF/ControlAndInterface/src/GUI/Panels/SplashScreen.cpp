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
			u8g2.drawXBM((128-KimchipsLogo100_width)/2, (64-KimchipsLogo100_height)/2, KimchipsLogo100_width,KimchipsLogo100_height,KimchipsLogo100_bits); 
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
			this->message = message;
		}
	}
}

