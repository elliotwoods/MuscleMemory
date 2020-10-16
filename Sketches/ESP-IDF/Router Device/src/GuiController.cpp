#include "GuiController.h"

//----------
GuiController & 
GuiController::X()
{
	static GuiController gui;
	return gui;
}

//----------
void
GuiController::init(U8G2 & screen, std::shared_ptr<Panel> rootPanel)
{
	this->screen = &screen;
	this->rootPanel = rootPanel;
	this->currentPanel = rootPanel;
}

//----------
void
GuiController::update()
{
	if(this->isDialButtonPressed) {
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
		this->isDialButtonPressed = false;
	}

	if(this->dialMovements != 0) {
		this->currentPanel->dial(this->dialMovements);
		this->dialMovements = 0;
	}

	this->currentPanel->update();

	this->screen->firstPage();
	do
	{
		this->currentPanel->draw(*this->screen);
	}
	while(this->screen->nextPage());
}

//----------
void
GuiController::dialButtonPressed()
{
	// printf("Dial button pressed \n");
	this->isDialButtonPressed = true;
}

//----------
void
GuiController::dialTurned(int8_t turns)
{
	// printf("Dial turned: %d\n", turns);
	this->dialMovements += turns;
}