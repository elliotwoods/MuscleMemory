#include "RegisterList.h"

namespace Panels {
    RegisterList::RegisterList()
    {
        auto & registry = Registry::X();
        for(auto & registerItem : registry.registers) {
            this->registers.push_back(&registerItem.second);
        }
    }

    void
    RegisterList::update()
    {
        // If we've scrolled off the list, wrap around
        if(this->cursorPosition < 0) {
            this->cursorPosition = this->registers.size() - 1;
        }
        else if(this->cursorPosition >= this->registers.size()) {
            this->cursorPosition = 0;
        }

        // If the cursor is outside the viewable area, then scroll the view
        if(this->cursorPosition < this->viewOffset)
        {
            this->viewOffset = this->cursorPosition;
        }
        else if(this->cursorPosition - this->viewOffset >= this->viewableItems)
        {
            this->viewOffset = this->cursorPosition - this->viewableItems + 1;
        }

        //printf("Cursor position : %d\n", cursorPosition);
    }

    void
    RegisterList::draw(U8G2 & screen)
    {
        const uint16_t rowHeight = 16;

        screen.setFont(u8g2_font_nerhoe_tr);

        // draw the list items
        for(size_t i=0; i<this->viewableItems; i++) {
            auto listIndex = i + this->viewOffset;
            if(listIndex >= this->registers.size()) {
                // If we've gone beyond the end of the list, then break
                break;
            }

            auto & registerItem = * this->registers[listIndex];
            
            
            char message[100];
            sprintf(message, "%s : %d", registerItem.name.c_str(), registerItem.value);
            screen.drawStr(20, i * rowHeight + rowHeight, message);
            //screen.drawText(registerItem.name, 20, i * 20);
        }

        // draw the cursor
        screen.drawStr(0, rowHeight * (this->cursorPosition - this->viewOffset) + rowHeight, ">");
    }

    bool
    RegisterList::buttonPressed()
    {
        return false;
    }

    void
    RegisterList::dial(int8_t delta)
    {
        this->cursorPosition += delta;
        //printf("dial by %d\n", delta);
    }
}
