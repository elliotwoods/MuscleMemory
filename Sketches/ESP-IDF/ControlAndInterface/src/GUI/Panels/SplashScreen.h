#pragma once

#include "../Panel.h"
#include "../../Registry.h"

namespace GUI {
	namespace Panels {
		class SplashScreen : public Panel
		{
		public:
			SplashScreen();
			void update() override;
			void draw(U8G2 &) override;
			bool buttonPressed() override;
			void dial(int8_t) override;

			void setMessage(const std::string&);
		private:
			std::string message;
		};
	}
}