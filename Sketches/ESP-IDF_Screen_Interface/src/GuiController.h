#pragma once

#include "Panel.h"
#include <memory>
#include <vector>

class GuiController {
public:
    static GuiController & X();
    void init(U8G2 & screen, std::shared_ptr<Panel> rootPanel);
    void update();

    void dialButtonPressed();
    void dialTurned(int8_t);
private:
    GuiController() {}
    std::shared_ptr<Panel> currentPanel = nullptr;
    std::vector<std::shared_ptr<Panel>> viewStack;

    // This will be shown when everything else exits
    std::shared_ptr<Panel> rootPanel;
    U8G2 * screen;
    
    int8_t dialMovements = 0;
    bool isDialButtonPressed = false;
};