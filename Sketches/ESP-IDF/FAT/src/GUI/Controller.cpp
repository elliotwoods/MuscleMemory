#include "Controller.h"
#include "U8G2HAL.h"

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

		this->rootPanel = rootPanel;
		this->currentPanel = rootPanel;
	}

	//----------
	void
	Controller::update()
	{
		// DIAL BUTTON
		{
			auto buttonPressed = gpio_get_level(GPIO_DIAL_BUTTON) == 0;
			if(buttonPressed) {
				if(this->currentPanel->buttonPressed()) {
					// The panel quit

					// Look through the view stack
					if(!viewStack.empty()) {
						// We are currently not in the root panel, so we want to go back
						viewStack.pop_back();

						if(viewStack.empty()) {
							// Now we should be in the root panel
							this->currentPanel = this->rootPanel;
						}
						else {
							// Now we want to be the previous panel which is not the root panel still
							this->currentPanel = viewStack.back();
						}
					}
				}
			}
		}
		
		// DIAL MOVEMENT
		{
			auto currentDialPosition = this->dial.getPosition();
			auto dialMovements = currentDialPosition - this->priorDialPosition;
			if(dialMovements != 0) {
				this->currentPanel->dial(dialMovements);
				this->priorDialPosition = currentDialPosition;
			}
		}
		
		// UPDATE THE CURRENT PANEL
		this->currentPanel->update();

		// DRAW THE CURRENT PANEL
		this->u8g2.firstPage();
		do
		{
			this->currentPanel->draw(this->u8g2);
		}
		while(this->u8g2.nextPage());
	}
}
