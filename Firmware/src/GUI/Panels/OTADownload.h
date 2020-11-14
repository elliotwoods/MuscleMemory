#pragma once

#include "GUI/Panel.h"

namespace GUI {
	namespace Panels {
		class OTADownload : public GUI::Panel {
		public:
			void update() override;
			void draw(U8G2 &) override;
			void buttonPressed() override;
			void dial(int8_t) override;

			std::string message;
		};
	}
}
