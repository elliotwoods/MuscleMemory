#pragma once

#include <stdint.h>
#include <stddef.h>

void canInit();
void canUpdate();
void canTask(void *);

void serialInit();
void serialUpdate();
void serialTask(void *);

size_t getCanToSerialCount();
size_t getSerialToCanCount();