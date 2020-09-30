#include <Arduino.h>
#include "sine_stepper.h"



void setup()
{
  Serial.begin(115200);
  motor_setup(32,33,14,27,200);
  set_speed(20);

  for(int i=0;i<2;i++)
  {
    step(10);
    delay(1000);
    //step(10);
    //delay(1000);    
    step(-13);
    delay(1000);
    //step(-7);
    //delay(1000);    
  }

}

void loop()
{


}