#pragma once

#include "../Panel.h"
#include "../../Registry.h"

namespace GUI {
	namespace Panels {
		class ShowID : public Panel
		{
		public:
			ShowID(); 
			void update() override;
			void draw(U8G2 &) override;
			bool buttonPressed() override;
			void dial(int8_t) override;
			bool shouldExit = false;
		private:
			bool editing = false;
			uint64_t startTime;
		};
	}
}