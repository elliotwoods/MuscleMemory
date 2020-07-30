from machine import Pin
from neopixel import NeoPixel
from time import sleep_ms
import math

np = NeoPixel(Pin(25), 4)

max_brightness = 100

cycles = 20
steps_per_cycle = 100
gamma = 2

def trail(d_theta):
    #note this needs to switch direction based on velocity
    d_theta /= math.pi * 2.0
    mod = 1.0 - math.fmod(d_theta + 10.0, 1.0)
    return mod

def cos(d_theta):
    brightness = math.cos(d_theta)
    brightness = max(brightness, 0)
    return brightness
        
for step in range(cycles * steps_per_cycle):
    theta = math.pi * 2.0 * float(step) / steps_per_cycle
    
    for led_index in range(4):
        d_theta = theta - (math.pi * led_index / 2)
        brightness = trail(d_theta)
        brightness = math.pow(brightness, gamma)
        
        brightness_int = int(brightness * max_brightness)
        brightness_int = max(brightness_int, 0)
        brightness_int = min(brightness_int, max_brightness)
        
        np[led_index] = (                
                brightness_int
                , brightness_int
                , brightness_int
            )
    print(theta)
    np.write()
    sleep_ms(10)

for l in range(4):
    np[l] = (0, 0, 0)
np.write()

