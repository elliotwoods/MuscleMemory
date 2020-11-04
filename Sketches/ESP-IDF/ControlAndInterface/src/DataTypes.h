#pragma once

#include "stdint.h"

typedef uint8_t PositionWithinStepCycle;
typedef uint16_t SingleTurnPosition;
typedef SingleTurnPosition EncoderReading;
typedef int32_t MultiTurnPosition;
typedef int32_t Velocity;
typedef int16_t Turns;
typedef int8_t Torque;
typedef uint16_t Current;
typedef int32_t Period; // us
typedef int32_t Frequency; // Hz
