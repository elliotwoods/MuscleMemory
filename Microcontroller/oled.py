from machine import Pin, I2C
import ssd1306
import time
import esp32

from pins import pins

def farenheit_to_celcius(farenheit_value):
    return (farenheit_value - 32) * 5 / 9

class Oled:        
    def __init__(self, reset = 16, scl = 15, sda = 4):        
        self.reset()
        self.show_temperature()

    def reset(self):
        pins.oled_reset.off()
        time.sleep_ms(50)
        pins.oled_reset.on()
        
        self.oled = ssd1306.SSD1306_I2C(128, 64, pins.i2c)
        
    def display(self, messages, print_messages = True):        
        y = 10
        self.oled.fill(0)
        for message in messages:
            self.oled.text(message, 0, y)
            y += 10
        self.oled.show()
        if print_messages:
            print(messages)
    
    def get_temperature(self):
        temperature_f = esp32.raw_temperature()
        temperature_c = farenheit_to_celcius(temperature_f)
        return temperature_c
    
    def show_temperature(self):    
        messages = []
        messages.append("Temperature:")
        messages.append("{0:.1f}C".format(self.get_temperature()))
        
        self.display(messages)
        
oled = Oled()