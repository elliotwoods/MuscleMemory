#pragma once

#include "Panel.h"
#include "../Devices/Dial.h"
#include <memory>
#include <vector>

namespace GUI {
	class Controller {
	public:
		static Controller & X();
		void init(std::shared_ptr<Panel> rootPanel);
		void update();

		void setRootPanel(std::shared_ptr<Panel> rootPanel);

		bool isDialButtonPressed() const;

		void pushPanel(std::shared_ptr<Panel>);
		void popPanel();
	private:
		Controller() {}
		void drawDisabledScreen();
		
		// The highest member of the view stack is currently shown
		std::vector<std::shared_ptr<Panel>> viewStack;
		
		// This will be shown when the view stack is empty
		std::shared_ptr<Panel> rootPanel;

		U8G2 u8g2;
		Devices::Dial dial;

		bool priorDialButtonPressed = false;
		int16_t priorDialPosition = 0;

		bool priorInterfaceDisabledScreenShown = false;
	};
}

