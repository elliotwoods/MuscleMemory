#pragma once

#include "../Panel.h"
#include "../Registry.h"

namespace Panels {
    class RegisterList : public Panel
    {
    public:
        RegisterList();
        void update() override;
        void draw(U8G2 &) override;
        bool buttonPressed() override;
        void dial(int8_t) override;
    private:
        std::vector<Registry::Register *> registers;

        const uint16_t viewableItems = 4;
        uint16_t cursorPosition = 0;
        uint16_t viewOffset = 0;
    };
}