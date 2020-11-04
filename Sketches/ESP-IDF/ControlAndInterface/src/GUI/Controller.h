#pragma once

#include "Panel.h"
#include "../Devices/Dial.h"
#include <memory>
#include <vector>

#define GPIO_DIAL_A GPIO_NUM_34
#define GPIO_DIAL_B GPIO_NUM_35
#define GPIO_DIAL_BUTTON GPIO_NUM_39

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
		
		std::shared_ptr<Panel> currentPanel = nullptr;
		std::vector<std::shared_ptr<Panel>> viewStack;

		// This will be shown when everything else exits
		std::shared_ptr<Panel> rootPanel;
		U8G2 u8g2;
		Devices::Dial dial;

		bool priorDialButtonPressed = false;
		int16_t priorDialPosition = 0;

		bool priorInterfaceDisabledScreenShown = false;
	};
}

