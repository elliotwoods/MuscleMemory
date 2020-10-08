#include <Arduino.h>
#include "sine_stepper.h"



void setup()
{
  Serial.begin(115200);
  motor_setup(32,33,14,27,200);
  set_speed(80);

  for(int i=0;i<5;i++)
  {
    step(600);
    delay(1000);
    
    step(-630);
    delay(1000);
  
  }

}

void loop()
{


}