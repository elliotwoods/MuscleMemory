#include "Lambda.h"

namespace GUI {
	namespace Panels {
		//----------
		void
		Lambda::update()
		{
			if(this->onUpdate) {
				this->onUpdate();
			}	
		}

		//----------
		void
		Lambda::draw(U8G2 & u8g2)
		{
			if(this->onDraw) {
				this->onDraw(u8g2);
			}	
		}

		//----------
		void
		Lambda::buttonPressed()
		{
			if(this->onButtonPressed) {
				this->onButtonPressed();
			}
		}

		//----------
		void
		Lambda::dial(int8_t movements)
		{
			if(this->onDial) {
				this->onDial(movements);
			}
		}
	}
}