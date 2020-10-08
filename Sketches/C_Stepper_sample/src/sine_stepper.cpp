#include "sine_stepper.h"

#include "driver/dac.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "sdkconfig.h"
#include <math.h>
#define PI       3.14159265

gpio_num_t MOTOR_PINS[4];
int _spr = 200;         // steps per revolution
int step_delay = 0;
int now_step = 0;

int sin_array[3600];    // the sine values' container
int sin_size = 3600;
int sin_range = 200;    //256;    // in range 0 - 255

bool debug = false;

void motor_setup(int pin_A,int pin_B,int pin_C,int pin_D,int SpR)
{
    MOTOR_PINS[0] = (gpio_num_t)pin_A;
    MOTOR_PINS[1] = (gpio_num_t)pin_B;
    MOTOR_PINS[2] = (gpio_num_t)pin_C;
    MOTOR_PINS[3] = (gpio_num_t)pin_D;
    _spr = SpR;

    // Set Stepper 4 Pins
    for(int i=0;i<4;i++)
    {
        gpio_reset_pin(MOTOR_PINS[i]);
        gpio_set_direction(MOTOR_PINS[i], GPIO_MODE_OUTPUT);
        gpio_set_level(MOTOR_PINS[i], 0);    
    }

    // set Vref Pins
    dac_output_enable(DAC_GPIO25_CHANNEL);
    dac_output_enable(DAC_GPIO26_CHANNEL);
    dac_output_voltage(DAC_GPIO25_CHANNEL, 0);
    dac_output_voltage(DAC_GPIO26_CHANNEL, 0);    

    // generate sine array
    sin_cal();
}

void sin_cal(void)
{
    for(int i=0;i<sin_size;i++)
    {
        double _val = sin((double)PI*i/ sin_size);
        sin_array[i] = sin_range * _val;
    }
}

void set_speed(int rpm)
{
    step_delay = 60 * 1000 * 1000 / (_spr * rpm);
    if(debug)  printf("-- RPM: %d. Step Delay: %d us.\n" , rpm, step_delay);
}

void step(int step_togo)
{
    int step_left = abs(step_togo);
    int direction = 1;
    if(step_togo<0)
    {
        direction = 0;
        //step_left--;
        now_step--;
    }

    if(debug)  printf("          Start (now %d.) Go to %d step\n",now_step, step_togo);

    int64_t last_bang = esp_timer_get_time();

    while(step_left>0)
    {
        int64_t _now = esp_timer_get_time();

        if(last_bang + step_delay <= _now)
        {   
            // go to next step
            step_left--;
            
            if(direction>0) now_step++;
            else            now_step--;
            if(step_left==0)
            {
                // let motor find the right angle in the end
                if(direction==0)    now_step++;
                
                int _phase = now_phase();
                int _vref[2] = {0,0};
                if(_phase==0||_phase==2)   _vref[1]=sin_range;
                else _vref[0]=sin_range;

                dac_output_voltage(DAC_GPIO25_CHANNEL, _vref[0]);
                dac_output_voltage(DAC_GPIO26_CHANNEL, _vref[1]);   

                

                if(debug) 
                {
                    printf("---------------End. phase %d now step %d, \n",_phase,now_step);
                    printf("------%d,%d \n",_vref[0],_vref[1]);
                }
                vTaskDelay(100 / portTICK_PERIOD_MS);   // hold in 100ms
                dac_output_voltage(DAC_GPIO25_CHANNEL, 0);
                dac_output_voltage(DAC_GPIO26_CHANNEL, 0);   
            }
            else
            {
                last_bang += step_delay;
            }
        }
        else
        {   
            // find the position
            int position = (_now - last_bang)*sin_size / (step_delay*2);
            if(position==0)    position++;
            int _phase = now_phase();
            if(direction>0) position += _phase*sin_size/2;
            else position = (_phase+1)*(sin_size/2)-position;
            //if(position%(sin_size/2)==0)   position--;

            // find sine value
            int _sine_array[4] = {0,0,0,0};
            int _vref[2] = {0,0};
            if(_phase==0)         // phase_0 (0,3)
            {
                _sine_array[0] = sin_array[position];
                _sine_array[3] = sin_array[sin_size/2+position];
                _vref[0] = _sine_array[0];
                _vref[1] = _sine_array[3];
            }
            else if(_phase==1)      // phase_1 (0,2)
            {
                _sine_array[0] = sin_array[position];
                _sine_array[2] = sin_array[position-sin_size/2];
                _vref[0] = _sine_array[0];
                _vref[1] = _sine_array[2];
            }
            else if(_phase==2)  // phase_2 (1,2)
            {
                _sine_array[1] = sin_array[position-sin_size];
                _sine_array[2] = sin_array[position-sin_size/2];
                _vref[0] = _sine_array[1];
                _vref[1] = _sine_array[2];
            }
            else                            // phase_3 (1,3)
            {
                _sine_array[1] = sin_array[position-sin_size];
                _sine_array[3] = sin_array[position-sin_size*3/2];
                _vref[0] = _sine_array[1];
                _vref[1] = _sine_array[3];
            }

            // set GPIO and DAC
            // GPIO
            for(int i=0;i<4;i++)
            {
                if(_sine_array[i]>0)
                {
                    gpio_set_level(MOTOR_PINS[i], 1); 
                }
                else
                {
                    //if(_sine_array[i]<0)    _sine_array[i]=0;
                    gpio_set_level(MOTOR_PINS[i], 0); 
                }   
            }
            if(_vref[0]<0)  _vref[0]=0;
            if(_vref[1]<0)  _vref[1]=0;
            

            //DAC
            dac_output_voltage(DAC_GPIO25_CHANNEL, _vref[0]);
            dac_output_voltage(DAC_GPIO26_CHANNEL, _vref[1]); 
            
            if(debug)  
            {
                printf("%d,---%d,%d,%d,%d .. phase %d Now %d   --- Vref %d,%d\n",
                    position, _sine_array[0],_sine_array[1],_sine_array[2],_sine_array[3],
                    _phase,now_step,_vref[0],_vref[1]);
            }
                
        }
    }
}



int now_phase(void)
{
    int _phase = now_step%4;
    if(_phase<0)    _phase+=4;
    return _phase;
}
