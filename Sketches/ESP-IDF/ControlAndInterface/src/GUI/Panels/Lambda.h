#pragma once

#include "../Panel.h"
#include "../../Registry.h"

#include <functional>

namespace GUI {
	namespace Panels {
		class Lambda : public Panel
		{
		public:
			void update() override;
			void draw(U8G2 &) override;
			void buttonPressed() override;
			void dial(int8_t) override;

			std::function<void()> onUpdate;
			std::function<void(U8G2&)> onDraw;
			std::function<void()> onButtonPressed;
			std::function<void(int8_t)> onDial;
		};
	}
}