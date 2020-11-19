#pragma once

#include "GUI/Panel.h"

namespace GUI {
	namespace Panels {
		class MarqueeText : public Panel {
		public:
			static void show(const std::string &);

			MarqueeText(const std::string &);
			void update() override;
			void draw(U8G2 &) override;
			void buttonPressed() override;
			void dial(int8_t) override;

			int speed = 4;
			bool inverted = false;
		private:
			std::string text;
			int position = 0;
			bool autoScrolling = true;
		};
	}
}