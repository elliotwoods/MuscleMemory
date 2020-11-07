from pins import pins
from machine import DAC
from time import sleep_us

class Stepper:
    def __init__(self):
        self.coils = [
            pins.motor_1
            , pins.motor_2
            , pins.motor_3
            , pins.motor_4
        ]
        self.vref_12 = DAC(pins.motor_vref_a)
        self.vref_34 = DAC(pins.motor_vref_b)
    
        self.sequence = [
            [1, 0, 0, 0]
            , [1, 1, 0, 0]
            , [0, 1, 0, 0]
            , [0, 0, 1, 0]
            , [0, 0, 1, 1]
            , [0, 0, 0, 1]
        ]
    
    def activate(self, index):
        values = self.sequence[index]
        for i in range(4):
            self.coils[i].on() if values[i] else self.coils[i].off()
        
    def step(self):
        self.vref_12.write(30)
        self.vref_34.write(30)
        for i in range(len(self.sequence)):
            self.activate(i)
            sleep_us(10000)
        self.vref_12.write(0)
        self.vref_34.write(0)
    
stepper = Stepper()