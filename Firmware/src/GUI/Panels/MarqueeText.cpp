#include "MarqueeText.h"
#include "GUI/Controller.h"

namespace GUI {
	namespace Panels {
		//----------
		void
		MarqueeText::show(const std::string & text)
		{
			printf(text.c_str());
			printf("\n");

			auto panel = std::make_shared<MarqueeText>(text);
			GUI::Controller::X().pushPanel(panel);
			while(!panel->shouldExit) {
				GUI::Controller::X().update();
				vTaskDelay(10 / portTICK_PERIOD_MS);
			}
		}

		//----------
		MarqueeText::MarqueeText(const std::string & text)
		:text(text)
		{

		}

		//----------
		void
		MarqueeText::update()
		{
			if(this->autoScrolling) {
				this->position += this->speed;
				const auto textWidth = 31 * this->text.size();
				if(this->position > textWidth) {
					this->shouldExit = true;
				}
			}
		}

		//----------
		void
		MarqueeText::draw(U8G2 & u8g2)
		{
			u8g2.setFont(u8g2_font_inb38_mr);

			auto xString = 32 - this->position;

			for(int i=0; i<this->text.size(); i++) {
				auto xChar = xString + i * 31;
				if(xChar < - 31) {
					continue;
				}
				if(xChar > 128) {
					break;
				}
				u8g2.drawGlyph(xChar, 55, this->text[i]);
			}

			if(this->inverted) {
				u8g2.setDrawColor(2);
				u8g2.drawBox(0, 0, 64, 128);
				u8g2.setDrawColor(1);
			}
		}

		//----------
		void
		MarqueeText::buttonPressed()
		{
			this->shouldExit = true;
		}

		//----------
		void
		MarqueeText::dial(int8_t movements)
		{
			this->autoScrolling = false;
			this->position += movements;
		}
	}
}