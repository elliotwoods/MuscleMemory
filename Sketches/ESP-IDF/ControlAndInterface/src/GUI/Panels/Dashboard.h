#pragma once

#include "../Panel.h"
#include "../../Registry.h"

namespace GUI {
	namespace Panels {
		class Dashboard : public Panel
		{
		public:
			Dashboard(); 
			void update() override;
			void draw(U8G2 &) override;
			bool buttonPressed() override;
			void dial(int8_t) override;
		};
	}
}