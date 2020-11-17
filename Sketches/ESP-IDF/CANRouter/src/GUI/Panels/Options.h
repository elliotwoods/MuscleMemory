#pragma once

#include "GUI/Panel.h"
#include <functional>
#include <vector>
#include <initializer_list>

namespace GUI {
	namespace Panels {
		class Options : public Panel {
		public:
			struct Option {
				std::function<void(char *)> text;
				std::function<void()> action;
			};

			Options(const std::string & title, const std::vector<Option> &);
			Options(const std::string & title, const std::initializer_list<Option> &);
			void update() override;
			void draw(U8G2 &) override;
			void buttonPressed() override;
			void dial(int8_t) override;
		private:
			std::string title;
			std::vector<Option> options;
			uint16_t selection = 0;
			uint16_t scroll = 0;
			const uint16_t viewableItems = 4;
		};
	}
}