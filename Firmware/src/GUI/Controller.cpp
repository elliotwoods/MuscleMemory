#include "Controller.h"
#include "U8G2HAL.h"
#include "Registry.h"

namespace GUI {
	//----------
	Controller & 
	Controller::X()
	{
		static Controller gui;
		return gui;
	}

	//----------
	void
	Controller::init(std::shared_ptr<Panel> rootPanel)
	{
		// Initialise the screen
		{
			u8g2_Setup_ssd1306_i2c_128x64_noname_1(this->u8g2.getU8g2()
				, U8G2_R0
				, u8g2_byte_hw_i2c_esp32
				, u8g2_gpio_and_delay_esp32);
			u8x8_SetPin(this->u8g2.getU8x8(), U8X8_PIN_RESET, GPIO_NUM_16);
			u8x8_SetI2CAddress(this->u8g2.getU8x8(), 0x3c);
			this->u8g2.begin();
		}

		// Initialise the dial
		{
			this->dial.init(GPIO_DIAL_A, GPIO_DIAL_B);
			gpio_set_direction(GPIO_DIAL_BUTTON, gpio_mode_t::GPIO_MODE_INPUT);
			this->priorDialPosition = this->dial.getPosition();
		}

		this->setRootPanel(rootPanel);
	}

	//----------
	void
	Controller::update()
	{
		// check if interface is disabled
		{
			static auto & registry = Registry::X();
			auto & isEnabled = registry.registers.at(Registry::RegisterType::InterfaceEnabled).value;
			if(isEnabled == 0) {
				if(!this->priorInterfaceDisabledScreenShown) {
					this->drawDisabledScreen();
					this->priorInterfaceDisabledScreenShown = true;
				}

				if(this->isDialButtonPressed()) {
					isEnabled = 1;
				}
				return;
			}
			else {
				this->priorInterfaceDisabledScreenShown = false;
			}
		}

		// Handle view exit events
		if(!this->viewStack.empty()) {
			if(this->viewStack.back()->shouldExit) {
				this->viewStack.pop_back();
			}
		}

		// Select current panel to show
		auto currentPanel = this->viewStack.empty()
			? this->rootPanel
			: this->viewStack.back();

		if(!currentPanel) {
			// No panel active
			return;
		}

		// DIAL BUTTON
		{
			auto buttonPressed = this->isDialButtonPressed();
			if(buttonPressed && !this->priorDialButtonPressed) {
				currentPanel->buttonPressed();
			}
			this->priorDialButtonPressed = buttonPressed;
		}
		
		// DIAL MOVEMENT
		{
			auto currentDialPosition = this->dial.getPosition();
			auto dialMovements = currentDialPosition - this->priorDialPosition;
			if(dialMovements != 0) {
				currentPanel->dial(dialMovements);
				this->priorDialPosition = currentDialPosition;
			}
		}
		
		// UPDATE THE CURRENT PANEL
		currentPanel->update();

		// DRAW THE CURRENT PANEL
		this->u8g2.firstPage();
		do
		{
			currentPanel->draw(this->u8g2);
		}
		while(this->u8g2.nextPage());
	}
	
	//----------
	void
	Controller::setRootPanel(std::shared_ptr<Panel> rootPanel)
	{
		this->rootPanel = rootPanel;
	}

	//----------
	bool
	Controller::isDialButtonPressed() const
	{
		return gpio_get_level(GPIO_DIAL_BUTTON) == 0;
	}

	//----------
	void
	Controller::pushPanel(std::shared_ptr<Panel> panel)
	{
		this->viewStack.push_back(panel);
	}

	//----------
	void
	Controller::popPanel()
	{
		if(!this->viewStack.empty()) {
			this->viewStack.pop_back();
		}
	}

	//----------
	void
	Controller::drawDisabledScreen()
	{
		this->u8g2.firstPage();
		do
		{
			this->u8g2.setFont(u8g2_font_nerhoe_tr);
			this->u8g2.setFontMode(0);
			this->u8g2.drawStr(10, 28, "Screen disabled");
			this->u8g2.drawStr(10, 38, "Press button to restore...");
			this->u8g2.drawFrame(0, 0, 128, 64);
		}
		while(this->u8g2.nextPage());
	}
}
