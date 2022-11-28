#pragma once

#include "../Panel.h"
#include "../../Registry.h"

#include <vector>

namespace GUI {
	namespace Panels {
		class RegisterList : public Panel
		{
		public:
			RegisterList();
			void update() override;
			void draw(U8G2 &) override;
			void buttonPressed() override;
			void dial(int8_t) override;
		private:
			std::vector<Registry::Register *> registers;

			const uint16_t viewableItems = 5;
			uint16_t cursorPosition = 0;
			uint16_t viewOffset = 0;
		};
	}
}