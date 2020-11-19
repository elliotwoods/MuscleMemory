#pragma once

#include "../Panel.h"
#include "../../Registry.h"

namespace GUI {
	namespace Panels {
		class ShowID : public Panel
		{
		public:
			static void show();
			
			ShowID(); 
			void update() override;
			void draw(U8G2 &) override;
			void buttonPressed() override;
			void dial(int8_t) override;
		private:
			bool editing = false;
			uint64_t startTime;
		};
	}
}