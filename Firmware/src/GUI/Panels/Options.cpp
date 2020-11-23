#include "Options.h"

namespace GUI {
	namespace Panels {
		//----------
		Options::Options(const std::string & title, const std::vector<Option> & options)
		: title(title)
		, options(options)
		{

		}

		//----------
		Options::Options(const std::string & title, const std::initializer_list<Option> & options)
		: title(title)
		, options(options)
		{

		}

		//----------
		void
		Options::update()
		{
			// Check if selection is viewable
			if(this->selection < this->scroll) {
				this->scroll = this->selection;
			}
			else if(this->selection > this->scroll + this->viewableItems - 1) {
				this->scroll = this->selection - this->viewableItems + 1;
			}
		}

		//----------
		void
		Options::draw(U8G2 & u8g2)
		{
			// Since it's interactive we draw a frame
			u8g2.drawFrame(0, 5, 128, 59);
			
			// Draw title
			u8g2.setFont(u8g2_font_10x20_mr);
			u8g2.drawStr(5, 13, this->title.c_str());

			// Draw options
			u8g2.setFont(u8g2_font_nerhoe_tr);
			char text[100];
			for(uint16_t i=0; i<this->viewableItems; i++) {
				uint16_t itemIndex = i + this->scroll;
				if(itemIndex < this->options.size()) {
					this->options[itemIndex].text(text);
					u8g2.drawStr(3, i * 12 + 26, text);

					// selected option
					if(itemIndex == this->selection) {
						if(this->options[itemIndex].action) {
							// Actionable options get inverse
							u8g2.setDrawColor(2);
							u8g2.drawBox(2, (i-1) * 12 + 26 + 2, 128-4, 12);
							u8g2.setDrawColor(1);
						}
						else {
							// Non-actionable options get outline
							u8g2.drawFrame(2, (i-1) * 12 + 26 + 2, 128-4, 12);
						}
						
					}
				}

			}
		}

		//----------
		void
		Options::buttonPressed()
		{
			if(this->onInteraction) {
				this->onInteraction();
			}

			if(this->selection < this->options.size()) {
				auto & action = this->options[this->selection].action;
				if(action) {
					action();
				}
			}
		}

		//----------
		void
		Options::dial(int8_t movements)
		{
			if(this->onInteraction) {
				this->onInteraction();
			}

			// Move selection
			auto newSelection = (int16_t) this->selection + (int16_t) movements;
			if(newSelection < 0) {
				this->selection = 0;
			}
			else if(newSelection > this->options.size() - 1) {
				this->selection = this->options.size() - 1;
			}
			else {
				this->selection = newSelection;
			}
		}
	}
}