from machine import Pin
from neopixel import NeoPixel
from time import sleep_ms
from math import sin

np = NeoPixel(Pin(25), 4)

max_brightness = 50

for theta_int in range(100):
    theta = float(theta_int) / 10.0
    for l in range(4):
        np[l] = (
                int(max_brightness / 2 * (1.0 + sin(theta + l*0.5 * 3 + 0)))
                , int(max_brightness / 2 * (1.0 + sin(theta + l*0.5 * 3 + 1)))
                , int(max_brightness / 2 * (1.0 + sin(theta + l*0.5 * 3 + 2)))
            )
        print(np[l])
    np.write()
    sleep_ms(30)

for l in range(4):
    np[l] = (0, 0, 0)
np.write()
