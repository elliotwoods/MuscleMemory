#pragma once
#include "stdint.h"
#include "driver/gpio.h"
#include "driver/pcnt.h"

class Dial {
public:
    static Dial & X();
    void init(gpio_num_t pinA, gpio_num_t pinB);
    int16_t getPosition();
private:
    pcnt_unit_t pcntUnit;
};

void initDial();