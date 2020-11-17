#pragma once

#include "U8g2lib.h"

namespace GUI {
	class Panel {
	public:
		virtual void update() = 0;
		virtual void draw(U8G2 &) = 0;
		virtual void buttonPressed() = 0;
		virtual void dial(int8_t) = 0; // returns true if this screen should exit
		bool shouldExit = false;
	};
}
