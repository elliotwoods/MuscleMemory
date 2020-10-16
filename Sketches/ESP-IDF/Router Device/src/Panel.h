#pragma once

#include "U8g2lib.h"

class Panel {
public:
    virtual void update() = 0;
    virtual void draw(U8G2 &) = 0;
    virtual bool buttonPressed() = 0;
    virtual void dial(int8_t) = 0; // returns true if this screen should exit
};